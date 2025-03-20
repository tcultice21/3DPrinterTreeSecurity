#Tree-based, Root-of-Trust Framework for 3D Printer Applications
## Built for SAMC21 Microcontrollers (Currently planning generalization to any MCU)

3D Printing Security design featuring the CAN bus. Capable of being both a farm network and internal 3D printer communication protocol with hierarchical-based, root-of-trust. Can use post-quantum or lightweight cryptography.

Current endpoint code focuses on SAMC21 direct control of Snapmaker linear modules and printer head. However, the code can be generalized to work with any capability list the user requires, given an update in this "firmware".

TODO List:
- Isolate SAMC21 Directives/Register access to generalize for all MCUs
- Build stronger callback structure with >1 capability/device list generation
- Build Marlin command conversion for easy P&P with software like Luban/Cura
