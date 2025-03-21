# Tree-based, Root-of-Trust Framework for 3D Printer Applications
## Built for SAMC21 Microcontrollers (Currently planning generalization to any MCU)

3D Printing Security design featuring the CAN-FD bus, however any communication protocol can work. Capable of being both a farm network and internal 3D printer communication protocol with hierarchical-based, root-of-trust. Can use post-quantum or lightweight cryptography.

Current endpoint code focuses on SAMC21 direct control of Snapmaker linear modules and printer head. However, the code can be generalized to work with any capability list the user requires, given an update in this "firmware". To do this, provide code for the callback functions that will define your low level interactions (motors, sensors, etc.).

TODO List:
- Isolate SAMC21 Directives/Register access to generalize for all MCUs
- Build stronger callback structure with >1 capability/device list generation
- Build Marlin command conversion at the root for easy P&P with software like Luban/Cura

Related to the Publications:
Cultice, T.; Clark, J.; Yang, W.; Thapliyal, H. A Novel Hierarchical Security Solution for Controller-Area-Network-Based 3D Printing in a Post-Quantum World. Sensors 2023, 23, 9886. [https://doi.org/10.3390/s23249886](https://doi.org/10.3390/s23249886).

Cultice, T., Clark, J., & Thapliyal, H. (2023, June). Lightweight Hierarchical Root-of-Trust Framework for CAN-Based 3D Printing Security. In Proceedings of the Great Lakes Symposium on VLSI 2023 (pp. 215-216). [https://doi.org/10.1145/3583781.3590324](https://doi.org/10.1145/3583781.3590324).
