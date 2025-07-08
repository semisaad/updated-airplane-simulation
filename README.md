# ✈️ Airplane Simulation

A simple 3D airplane simulation built in C using Raylib and RayGUI.

## 📦 Features
- 3D terrain from a heightmap
- Multiple planes (add/remove)
- Autopilot and manual control
- First-person and free camera modes
- Landing zone detection
- Tutorial image popup

## 🎮 Controls
- `W/S` — Up/Down  
- `A/D` — Yaw  
- `UP/DOWN` — Pitch  
- `LEFT/RIGHT` — Roll  
- `Left Shift` — Move Forward  
- `Space` — Boost  
- `F` — Toggle First-person  
- `R` — Toggle Free Camera  
- `C` — Toggle Autopilot  
- `?` Button — Show tutorial  
- `Y/N` — Restart / Exit on landing  

## 🔧 Build
Requires **Raylib** and **RayGUI**.

```bash
gcc main.c -o AirplaneSim -lraylib -lm -ldl -lpthread -lGL -lX11
./AirplaneSim
