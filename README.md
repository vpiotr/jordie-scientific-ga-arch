# Overview
Software archive for genetic algorithm libraries.

# DISCLAIMER 
This is just a code dump, this software is not supposed to be in a working state and is provided as-is.
Some parts of the original software have been omitted - if you want to use it, you need to remove references to missing parts or use your own implementation instead.
See LICENSE.txt for further details.

# Library index

## Scientific - genetic algorithms
* ga           - genetic algorithm library

# Current software state
Source codes are compatible with C++98 and designed to work with Win32 platform (VS).

Large portion of the code is based on scDataNode data type which is a variant type compatible with `xnode`:
https://github.com/vpiotr/xnode

Most of the code base is using `scString` instead of std::string, but in fact it's the same data type.

Some libraries require Boost. 
Some libraries still use std::auto_ptr (C++98).

# TODO
If you want to continue work on these libraries, I would recommend:
* switching to at least C++11 
* replace std::auto_ptr by std::unique_ptr
* switch from Boost regular expressions to C++11 regular expressions in wildcard lib
* switch from dnode to better organized xnode or to STL-like interfaces where possible
* switch to Boost class naming standard 
* eliminate dependency on wxWidgets (process)
* "include" preprocessor commands need to be corrected 
 

