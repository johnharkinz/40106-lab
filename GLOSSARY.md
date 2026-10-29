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

## Inverter controls

**Control**:
A musical setting that is mapped onto an inverter's physical values (feedback resistance, capacitance, thresholds). Patches and the instrument's knobs speak in controls; the circuit model speaks in physical values.
_Avoid_: parameter (when meaning a musical setting), macro

**Rate**:
The continuous control that sets how fast an inverter oscillates, standing in for a pot on its feedback resistance.
_Avoid_: pitch, frequency, speed

**Range**:
The stepped control that selects an inverter's capacitor, moving its whole Rate span up or down.
_Avoid_: octave, register, multiplier

**Hysteresis**:
The gap between an inverter's lower and upper thresholds.
_Avoid_: threshold width, deadband

**Asymmetry**:
How far an inverter's thresholds are shifted together away from mid-supply, changing its duty cycle.
_Avoid_: bias, offset, pulse width

## Output

**Level**:
How loud one inverter is in the mix. It belongs to the mixer, not the inverter: links always see an inverter's full output.
_Avoid_: volume, gain, amplitude (when meaning the mix setting)
