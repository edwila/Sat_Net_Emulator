# Sat_Net_Emulator
Emulation of satellite networks (i.e., Starlink, LEO, etc.) built on similar models.

Satellite positions stored as Vector3 (relative to center of Earth, <0, 0, 0>) as are User positions.

Distances are approximately equivalent to real-life, simulating real-world latency. Similarly, data movement through different environments is approximately equivalent to real-life; most data is read/written from/to shared memory in a circular lock-free buffer. This simulates data being fed to hardware (i.e., from a sensor issuing an IRQ).


[Demo video](https://youtu.be/UiHcEGuiJZM?si=XgOSDZReG553awqs)
