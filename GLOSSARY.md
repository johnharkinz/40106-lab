# 40106 Lab

A virtual CMOS noise box: a simulation of interacting CD40106 Schmitt-trigger RC oscillators, intended to run eventually as standalone firmware on the M-VAVE FM-1.

## Language

**M-VAVE FM-1**:
The hardware synthesiser this instrument targets as firmware.
_Avoid_: M-Wave, FM1, the box

**Inverter**:
One virtual CD40106 Schmitt-trigger stage with its own capacitor and feedback resistance, forming an oscillator.
_Avoid_: oscillator (when meaning the modelled stage), voice, gate

**Link**:
A directed connection from one inverter to another that lets the source influence the target.
_Avoid_: connection, patch cord, route

**Sync link**:
A link through which the source output forces the target's capacitor toward a level, producing hard sync.
_Avoid_: hard-sync connection

**Coupling link**:
A link where the source output feeds the target's capacitor through a virtual resistance.
_Avoid_: resistive connection, modulation
