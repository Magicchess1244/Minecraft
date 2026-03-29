# 🧱 Voxel-Engine-SDL3

A high-performance, ultra RAM-efficient **Minecraft-style sandbox engine** built from scratch in **C++**.
This project focuses on aggressive optimization, leveraging **multithreading** and the modern **SDL3 ecosystem** to push performance on modest hardware.

---

## 🚀 Technical Highlights

### 🧵 Multithreaded Terrain Generation

World generation and mesh rebuilding are fully decoupled from the main render loop, delivering a smooth **60+ FPS** experience even during heavy world streaming.

### 🧠 Memory Efficiency

Designed with custom data structures and bit-packing techniques to achieve a **minimal memory footprint**, enabling large render distances without heavy RAM usage.

### ⚙️ Modern SDL3 Stack

Built on the latest **SDL3** libraries for improved:

* Window management
* Input handling
* Rendering stability

---

## ✨ Core Features

### 🌍 World Systems

* **Advanced Texturing**
  Texture atlas system with efficient lighting (Ambient Occlusion + sunlight)

* **Save / Load System**
  Persistent worlds so players can keep their progress

---

### 🎮 Gameplay Mechanics

* **Crafting System**
  Fully functional crafting pipeline

* **Smelting System** *(WIP)*
  Core logic implemented, UI improvements ongoing

* **Extensive Block Library**
  Supports a wide range of block types and behaviors

---

### 🌐 Multiplayer

* Built-in networking support for multiplayer sessions

---

## 🛠 Tech Stack

| Dependency     | Purpose                                 |
| -------------- | --------------------------------------- |
| **C++20**      | Core engine logic and architecture      |
| **SDL3**       | Windowing, input, and low-level systems |
| **SDL3_image** | Texture loading and processing          |
| **SDL3_ttf**   | UI rendering and fonts                  |

---

## 🏗 Installation

### ✅ Prerequisites

Make sure you have the following installed:

* SDL3
* SDL3_image
* SDL3_ttf

> ⚠️ SDL3 is still cutting-edge — ensure your include and library paths are correctly configured.

---

### 🔧 Build from Source

```bash
git clone https://github.com/yourusername/Voxel-Engine-SDL3.git
cd Voxel-Engine-SDL3

mkdir build
cd build
cmake ..
make
```

---

## 📈 Development Roadmap

* [x] Multithreaded Chunk Loading
* [x] SDL3 Migration
* [ ] Finalize Smelting UI & Logic
* [ ] Implement Water Physics
* [ ] Optimize Multiplayer Latency

---

## 📄 License

This project is licensed under the **MIT License**.
See the `LICENSE` file for more details.

---

## 💡 Notes

This engine is built as a **performance-first project**, exploring:

* Low-level memory optimization
* Efficient chunk rendering
* Scalable voxel systems

---

## ⭐ Contributing

Contributions, ideas, and optimizations are welcome!
Feel free to open issues or submit pull requests.

---
