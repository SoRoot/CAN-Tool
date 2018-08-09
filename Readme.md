#CAN Tool#

###Abstract###

This Code for a testing tool is based on a Renesas controller - the Synergy SK-S7G2 - which, if transmitted to that very controller, is able to send CAN signals, that are configurable via an UART port. The UART port is connected to a computer via a serial interface and provides the parameters for the CAN signal to be emulated via this interface. Thus a CAN signal is simulated in the CAN module of the Synergy controller and sent cyclically.