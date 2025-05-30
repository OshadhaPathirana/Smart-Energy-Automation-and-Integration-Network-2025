# ⚡ Smart Solar Inverter System (SEIAN)

[![License: CC BY-NC 4.0](https://img.shields.io/badge/License-CC%20BY--NC%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc/4.0/)

A cutting-edge smart inverter system designed for integration into Smart Microgrids and the Smart National Grid. Our solution leverages IoT, advanced control systems, and machine learning to create an adaptive, fault-resilient energy platform.

## 📸 Project Snapshots
<table>
  <tr>
    <td align="center">
      <img src="Images/20250411_210455.jpg" alt="Team Photo" width="90%"/><br/>
      <sub><b>👨‍💻 Our Team</b></sub>
    </td>
    <td align="center">
      <img src="Images/20250322_183350.jpg" alt="Product View" width="90%"/><br/>
      <sub><b>💡 Product View</b></sub>
    </td>
  </tr>
</table>

### 🔧 Inverter Internals

<table>
  <tr>
    <td align="center">
      <img src="Images/20250314_214203.jpg" alt="Inverter Internals 1" width="75%"/><br/>
      <sub><b>🔧 Internal View 1</b></sub>
    </td>
    <td align="center">
      <img src="Images/20250314_215549.jpg" alt="Inverter Internals 2" width="75%"/><br/>
      <sub><b>🔧 Internal View 2</b></sub>
    </td>
  </tr>
</table>

---

## 🚀 Overview

**SEIAN** (Smart Energy IoT-Aware Network) is a smart inverter designed to:

- Monitor the grid status in real-time  
- Share operational data with a central server  
- Perform autonomous grid fault detection and correction  
- Support seamless connection to grid or microgrid environments

## 🧠 System Architecture

Our smart inverter system includes:

### 🎯 Local Inverter Node (SEIAN Unit)

- **AC Measurement Circuitry**:  
  - Measures voltage, current, frequency, power factor, harmonics, and phase.
- **Control Processor**:  
  - Calculates parameters and generates a synchronized sine wave for grid connection.
- **SPWM Control**:  
  - Sinusoidal Pulse Width Modulation (SPWM) via H-Bridge with 4 IGBTs and driver ICs.
- **Microcontroller**:  
  - Low-level registry-based control (to be upgraded to FPGA-based for optimization).
- **Communication Subsystem**:  
  - Overseen by a Raspberry Pi, communicates to central server via HTTP protocol.

### 🖥️ Central Server

- Collects real-time data from all SEIAN units  
- Runs ML models to:
  - Detect faults (e.g., voltage sags, harmonic distortions)
  - Classify them using a neural network trained on historical/simulated grid data  
  - Send corrective commands to inverter units (adjusting voltage, frequency, phase)

---

## 🛠 Current Status

✅ Inverter hardware prototype completed  
✅ Backend and database system integrated  
⚠️ PID feedback and filtering still under development  
⚠️ ML model in training phase  
⚠️ Cybersecurity layer in planning  
🔜 FPGA-based control and optimization in development

---

## 📍 Future Work

- Upgrade microcontroller-based control to an FPGA for faster and more reliable SPWM signal generation.  
- Improve PID control stability and filtering.  
- Complete cybersecurity and encryption layers for safe communication.  
- Finalize and deploy ML model to live test environments.

---

## 📫 Contact

**Dyson Sphare** – Project Lead: **Rusula Oshadha**  
Electronics and Telecommunication Engineering, University of Moratuwa  
**LinkedIn:** [https://www.linkedin.com/in/oshadhapathirana/](https://www.linkedin.com/in/oshadhapathirana/)

---
