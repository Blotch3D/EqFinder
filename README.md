# EqFinder
Finds equations given a list of dimensioned constants in a config file (eqfinder.txt)

Written in 1989. 
Ported to C++ in 1994.
Multithreading added in 1998.

This program takes a list of constants you specify in the config file (eqfinder.txt)
and finds any valid equations relating those constants.

The units (dimensions) of each constant (meters, kilograms, amperes,
seconds) are honored when testing equations for validity.

That is, the program will try many possible equations containing the
constants you specify, but display only those equations in
which the units of the constants all cancel.

The program tries equations of the form:

K1^E1  *  K2^E2  *  K3^E3 = unitless magnitude?

Where

Kn    is a constant

En    is an integral exponent.

For example, if you give the program the permittivity (p) and
permeability (u) of free space, and the speed of light (c), and
you specify that exponents should be tried from -2 to 2,
then the program will test the following equations for validity:

p^-2  *  u^-2  *  c^-2    =   unitless magnitude?

p^-2  *  u^-2  *  c^-1    =   unitless magnitude?

p^-2  *  u^-2  *  c^0     =   unitless magnitude?

p^-2  *  u^-2  *  c^1     =   unitless magnitude?

p^-2  *  u^-2  *  c^2     =   unitless magnitude?

p^-2  *  u^-1  *  c^-2    =   unitless magnitude?

p^-2  *  u^-1  *  c^-1    =   unitless magnitude?

 --etc--

For example if given the permittivity and permeability of free space, within a few milliseconds the program will discover Maxwell's solution for the
speed of light:

p^-1  *  u^-1  *  c^2    ==   1.000 (no units)

...but it has no proof, of course!
