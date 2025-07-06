#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>

// RayGUI implementation
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#ifndef MAP_DIFFUSE
    #define MAP_DIFFUSE MATERIAL_MAP_ALBEDO
#endif

#define MAX_PLANES 4

// A structure to hold all data for a single plane
typedef struct {
    float x, y, z;
    float pitch, roll, yaw;
    Matrix transform;
    bool active;
    bool autopilot;
} Plane;

// A helper function to initialize a new plane at a specific location
void InitPlane(Plane *plane, float x, float y, float z)
{
    plane->x = x;
    plane->y = y;
    plane->z = z;
    plane->pitch = 0.0f;
    plane->roll = 0.0f;
    plane->yaw = 0.0f;
    plane->active = true;
    plane->autopilot = false;
    plane->transform = MatrixIdentity();
}

// A helper function to reset the game to its initial state
void ResetGame(int *planeCount, int *activePlaneIndex, Plane *planes, float spawnX, float spawnY, float spawnZ)
{
    *planeCount = 1;
    *activePlaneIndex = 0;
    for (int i = 0; i < MAX_PLANES; i++) planes[i].active = false;
    InitPlane(&planes[0], spawnX, spawnY, spawnZ);
}


int main(void)
{
    // Initialize window (can be resized)
    InitWindow(1920, 1080, "Airplane Simulation");
    SetTargetFPS(60);

    // --- Load terrain ---
    Image hm = LoadImage("Great Lakes/Height-Map.png");
    if (!hm.data) { printf("Failed to load heightmap\n"); CloseWindow(); return 1; }
    ImageResize(&hm, hm.width/2, hm.height/2);
    Mesh terrainMesh = GenMeshHeightmap(hm, (Vector3){1000,350,1000});
    Model terrain     = LoadModelFromMesh(terrainMesh);
    UnloadImage(hm);
    Texture2D terrainTex = LoadTexture("Great Lakes/Diffuse-Map.png");
    terrain.materials[0].maps[MAP_DIFFUSE].texture = terrainTex;

    // Optional saturation shader
    Shader sat = LoadShader(0, "Assets/saturation.fs");
    float satVal = 3.0f;
    SetShaderValue(sat, GetShaderLocation(sat, "saturation"), &satVal, SHADER_UNIFORM_FLOAT);
    terrain.materials[0].shader = sat;

    // --- Load plane model ---
    Model planeModel = LoadModel("Assets/plane.obj");
    Texture2D planeTex = LoadTexture("Assets/An2_aeroflot.png");
    planeModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = planeTex;

    // --- Load runway ---
    Texture2D runwayTex = LoadTexture("Assets/runway_texture.png");
    Mesh runwayMesh = GenMeshPlane(6.0f, 24.0f, 1, 1);
    Model runway = LoadModelFromMesh(runwayMesh);
    runway.materials[0].maps[MAP_DIFFUSE].texture = runwayTex;

    // --- Audio ---
    InitAudioDevice();
    Music engineSound = LoadMusicStream("Assets/airplane-sound.mp3");
    PlayMusicStream(engineSound);

    // --- Spawn setup ---
    const float spawnX = -1000.0f, spawnY = 5500.0f, spawnZ = 19000.0f;
    const float planeSeparation = 150.0f; // ADDED: Distance between spawning planes
    Matrix scaleM  = MatrixScale(0.005f, 0.005f, 0.005f);
    Matrix corrM   = MatrixRotateY(PI/2);

    // --- Simulation state ---
    Plane allPlanes[MAX_PLANES] = { 0 };
    int planeCount = 0;
    int activePlaneIndex = 0;

    bool firstPerson = false, freeCam = false, gameOver = false;

    // --- Camera ---
    Camera camera = { 0 };
    camera.position   = (Vector3){0, 60, 120};
    camera.target     = (Vector3){0, 10,  0};
    camera.up         = (Vector3){0, 1,   0};
    camera.fovy       = 8.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // --- Landing spot ---
    Vector3 landingSpot     = {100.0f, 27.458f, 1.0f};
    const float collisionThreshold = 3.0f;

    // --- Menu flags ---
    bool inMenu      = true;
    bool gameStarted = false;

    // --- Main loop ---
    while (!WindowShouldClose())
    {
        int W = GetScreenWidth();
        int H = GetScreenHeight();

        if (gameStarted) UpdateMusicStream(engineSound);

        if (gameStarted && !gameOver)
        {
            if (IsKeyPressed(KEY_F)) firstPerson = !firstPerson;
            if (IsKeyPressed(KEY_R)) freeCam     = !freeCam;
            if (IsKeyPressed(KEY_C) && planeCount > 0)
            {
                allPlanes[activePlaneIndex].autopilot = !allPlanes[activePlaneIndex].autopilot;
            }
        }
        
        if (gameStarted && !gameOver && planeCount > 0)
        {
            for (int i = 0; i < planeCount; i++)
            {
                Plane *p = &allPlanes[i]; 

                const float groundAlt = 6000.0f;
                const float constantGravity = 0.5f;
                float speedFactor = 1.0f;
                if (p->y <= groundAlt)
                {
                    p->y -= constantGravity;
                    speedFactor = 0.5f;
                }
                Vector3 forward = { sinf(DEG2RAD * p->yaw), 0, cosf(DEG2RAD * p->yaw) };
                
                if (p->autopilot)
                {
                    p->x += forward.x * 40.5f * speedFactor;
                    p->z += forward.z * 40.5f * speedFactor;
                    p->y += sinf(DEG2RAD * p->pitch) * 20.5f * speedFactor;
                }

                if (i == activePlaneIndex)
                {
                    if (IsKeyDown(KEY_LEFT_SHIFT))
                    {
                        p->x += forward.x * 40.5f * speedFactor;
                        p->z += forward.z * 40.5f * speedFactor;
                        p->y += sinf(DEG2RAD*p->pitch) * 20.5f * speedFactor;
                    }
                    if (IsKeyDown(KEY_SPACE))
                    {
                        p->x += forward.x * 50.0f * speedFactor;
                        p->z += forward.z * 50.0f * speedFactor;
                    }
                    if (IsKeyDown(KEY_W)) p->y += 12.0f * speedFactor;
                    if (IsKeyDown(KEY_S)) p->y -= 12.0f * speedFactor;

                    if (IsKeyDown(KEY_DOWN))  p->pitch += 0.2f;
                    else if (IsKeyDown(KEY_UP)) p->pitch -= 0.2f;
                    else if (p->pitch > 0.2f)      p->pitch -= 0.2f;
                    else if (p->pitch < -0.2f)     p->pitch += 0.2f;

                    if (IsKeyDown(KEY_D)) p->yaw -= 0.4f;
                    if (IsKeyDown(KEY_A)) p->yaw += 0.4f;

                    if (IsKeyDown(KEY_LEFT))  p->roll -= 0.7f;
                    else if (IsKeyDown(KEY_RIGHT)) p->roll += 0.7f;
                    else if (p->roll > 0.0f) p->roll -= 0.3f;
                    else if (p->roll < 0.0f) p->roll += 0.3f;
                }

                p->x = Clamp(p->x, -189900.0f,  9900.0f);
                p->z = Clamp(p->z, -9900.0f,   189900.0f);
                p->y = Clamp(p->y, 5500.0f,   200000.0f);
            }
        }

        if (gameStarted)
        {
            for (int i = 0; i < planeCount; i++)
            {
                if (allPlanes[i].active)
                {
                    Matrix rotM   = MatrixRotateXYZ((Vector3){DEG2RAD*allPlanes[i].pitch, DEG2RAD*allPlanes[i].yaw, DEG2RAD*allPlanes[i].roll});
                    Matrix transM = MatrixTranslate(allPlanes[i].x, allPlanes[i].y, allPlanes[i].z);
                    allPlanes[i].transform = MatrixMultiply(rotM, MatrixMultiply(transM, MatrixMultiply(corrM, scaleM)));
                }
            }
            
            if (!gameOver && planeCount > 0)
            {
                Plane *activePlane = &allPlanes[activePlaneIndex];
                Vector3 planePos = { activePlane->transform.m12, activePlane->transform.m13, activePlane->transform.m14 };
                if (Vector3Distance(planePos, landingSpot) < collisionThreshold)
                {
                     gameOver = true;
                     activePlane->autopilot = false; 
                }
            }

            if (!freeCam && planeCount > 0)
            {
                Plane *cameraFocusPlane = &allPlanes[activePlaneIndex];
                Vector3 ppos = { cameraFocusPlane->transform.m12, cameraFocusPlane->transform.m13, cameraFocusPlane->transform.m14 };
                if (firstPerson)
                {
                    camera.position = Vector3Add(ppos, (Vector3){0,0.5f,0});
                    Matrix pr = MatrixRotateXYZ((Vector3){DEG2RAD*cameraFocusPlane->pitch, DEG2RAD*cameraFocusPlane->yaw, DEG2RAD*cameraFocusPlane->roll});
                    Matrix cm = MatrixMultiply(pr, corrM);
                    camera.target = Vector3Add(camera.position, Vector3Transform((Vector3){0,0,1}, cm));
                    camera.up     = Vector3Transform((Vector3){0,1,0}, cm);
                }
                else
                {
                    Matrix fr = MatrixRotateXYZ((Vector3){DEG2RAD*cameraFocusPlane->pitch, DEG2RAD*cameraFocusPlane->yaw, 0});
                    Vector3 offset = Vector3Transform((Vector3){-15,2.5,0}, fr);
                    Vector3 desired = Vector3Add(ppos, offset);
                    camera.position = Vector3Lerp(camera.position, desired, 0.1f);
                    camera.target   = ppos;
                }
            }
            else
            {
                UpdateCamera(&camera, CAMERA_FREE);
            }
        }

        BeginDrawing();
          ClearBackground(inMenu ? RAYWHITE : SKYBLUE);

          if (inMenu)
          {
              DrawText("Airplane Simulation", W/2 - MeasureText("Airplane Simulation", 40)/2, H/2 - 120, 40, DARKBLUE);

              if (GuiButton((Rectangle){W/2 -100, H/2 -40, 200, 50}, "Start"))
              {
                  inMenu      = false;
                  gameStarted = true;
                  gameOver    = false;
                  ResetGame(&planeCount, &activePlaneIndex, allPlanes, spawnX, spawnY, spawnZ);
              }
              if (GuiButton((Rectangle){W/2 -100, H/2 +20, 200, 50}, "Exit"))
              {
                  CloseWindow();
                  return 0;
              }
          }
          else
          {
              BeginMode3D(camera);
                DrawModel(terrain, (Vector3){-50,0,-50}, 1.0f, WHITE);
                DrawGrid(500,1);
                DrawModel(runway,  (Vector3){95,27.458f,15}, 1.0f, WHITE);

                for (int i = 0; i < planeCount; i++)
                {
                    if (allPlanes[i].active)
                    {
                        planeModel.transform = allPlanes[i].transform;
                        DrawModel(planeModel, (Vector3){0,0,0}, 1.0f, WHITE);
                    }
                }

                DrawSphere(landingSpot, 1.0f, RED);
              EndMode3D();

              DrawFPS(960,10);
              
              if (planeCount > 0)
              {
                  char coordText[128];
                  Plane *activePlane = &allPlanes[activePlaneIndex];
                  sprintf(coordText, "X: %.1f, Y: %.1f, Z: %.1f", activePlane->x, activePlane->y, activePlane->z);
                  int textWidth = MeasureText(coordText, 20);
                  DrawText(coordText, W - textWidth - 10, 10, 20, WHITE);
              }
              
              if (planeCount > 0 && allPlanes[activePlaneIndex].autopilot)
              {
                  int textWidth = MeasureText("AUTOPILOT ON", 30);
                  DrawText("AUTOPILOT ON", W/2 - textWidth/2, 40, 30, LIME);
              }

              Rectangle addBtn = { 20, 20, 140, 40 };
              // --- MODIFIED: The logic for adding a new plane ---
              if (GuiButton(addBtn, "Add Plane"))
              {
                  if (planeCount < MAX_PLANES)
                  {
                      // Calculate the offset spawn position for the new plane
                      float newX = spawnX + (planeCount * planeSeparation);
                      
                      // Initialize the new plane at the offset location
                      InitPlane(&allPlanes[planeCount], newX, spawnY, spawnZ);
                      planeCount++;
                  }
              }

              for (int i = 0; i < planeCount; i++)
              {
                  Rectangle letterBtn = { addBtn.x, addBtn.y + addBtn.height + 10 + (i * (addBtn.height + 10)), addBtn.width - 45, addBtn.height };
                  Rectangle removeBtn = { letterBtn.x + letterBtn.width + 5, letterBtn.y, 40, letterBtn.height };
                  
                  char label[16];
                  if (planeCount > 0 && i == activePlaneIndex) sprintf(label, "[Plane %c]", 'A' + i);
                  else sprintf(label, "Plane %c", 'A' + i);
                  
                  if (GuiButton(letterBtn, label))
                  {
                      activePlaneIndex = i;
                      freeCam = false; 
                  }
                  
                  if (GuiButton(removeBtn, "X"))
                  {
                      bool removedActive = (i == activePlaneIndex);

                      for (int j = i; j < planeCount - 1; j++)
                      {
                          allPlanes[j] = allPlanes[j+1];
                      }
                      planeCount--;
                      
                      if (planeCount > 0)
                      {
                          if (removedActive) activePlaneIndex = 0;
                          else if (i < activePlaneIndex) activePlaneIndex--;
                      }
                  }
              }

              if (gameOver)
              {
                  DrawRectangle(0,0,W,H, Fade(BLACK, 0.5f));
                  DrawText("LANDED!", W/2 - MeasureText("LANDED!", 40)/2, H/2 - 50, 40, WHITE);
                  DrawText("Press Y to Play Again or N to Exit", W/2 - MeasureText("Press Y to Play Again or N to Exit", 20)/2, H/2 + 10, 20, WHITE);
                  if (IsKeyPressed(KEY_Y))
                  {
                      gameOver = false;
                      ResetGame(&planeCount, &activePlaneIndex, allPlanes, spawnX, spawnY, spawnZ);
                  }
                  if (IsKeyPressed(KEY_N)) break;
              }
          }
        EndDrawing();
    }

    // --- Cleanup ---
    CloseAudioDevice();
    UnloadMusicStream(engineSound);
    UnloadModel(terrain);     UnloadTexture(terrainTex); UnloadShader(sat);
    UnloadModel(planeModel);  UnloadTexture(planeTex);
    UnloadModel(runway);      UnloadTexture(runwayTex);
    CloseWindow();

    return 0;
}