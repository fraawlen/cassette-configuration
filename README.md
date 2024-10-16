<p align="center"><img src="./extras/banner.svg"></p>

Cassette Configuration (CCFG) is a configuration language and parser library featuring array based values and short s-like expressions based functions. The language's syntax aims to be both human-readable and easy to parse. Yet provides enough tools to the end user to create branching and dynamic configurations that can be modified and reloaded on the fly.

The library is free and open-source software licensed under the [LGPL-3.0](https://www.gnu.org/licenses/lgpl-3.0.en.html). It's made to run on modern POSIX-compliant systems.

>[!WARNING]
>This repository is getting archived because it has been merged into a single repository with other Cassette libraries. The library now lives here: https://github.com/fraawlen/cassette

Language Features
-----------------

- comments
- user-defined sections
- user-defined variables
- program-defined parameters
- arithmetic operations
- string operations
- color operations
- iteration loops
- conditionals
- child file inclusion
- [more](./doc/spec.md)

Dependencies
------------

- Tools :

	- C99 compiler with a stdlib + POSIX 200809L
	- Make
	- [AFL++](https://aflplus.plus/) (Optional, only needed for [fuzzing](#fuzzing))

- First-party libraries :

	- [Cassette-Objects (COBJ)](/../../../../fraawlen/cassette-objects)

Installation
------------

First, edit the makefile if you want to change the installation destinations. These are represented by the variables `DIR_INSTALL_INC` and `DIR_INSTALL_LIB` for the public API headers and library files respectively. By default, they are set to `/usr/include/cassette/` and `/usr/lib`.
Then, build and install COBJ with the following commands :

```
make
make install
```

After these steps, both a shared binary and static archive will be generated and installed on your system. Examples will also be built and placed under `build/bin`.

Usage
-----

Add this include to get access to the library functions :

```
#include <cassette/ccfg.h>
```

As well as this compilation flag :

```
-lccfg
```

Minimal Example
---------------

The following code snippet shows a minimal example of the library usage. When compiled and run, it will look for `/tmp/data.conf` or `data.conf` and load their contents. It then attempts to fetch a resource named `property` under the namespace `namespace`, and if found, prints its values.

```c
#include <stdio.h>
#include <cassette/ccfg.h>

int
main(void)
{
	ccfg *cfg = ccfg_create();

	ccfg_push_source(cfg, "/tmp/data.conf"); /* primary  source */
	ccfg_push_source(cfg,      "data.conf"); /* fallback source */
	ccfg_load(cfg);

	ccfg_fetch(cfg, "namespace", "property");
	while (ccfg_iterate(cfg))
	{
		printf("%s\n", ccfg_resource(cfg));
	}

	return 0;
}
```

A matching minimal CCFG configuration in `/tmp/data.conf` or `data.conf` will then look like this :

```
namespace property value_A value_B
```

Output :

```
value_A
value_B
```

Check out the `examples` directory for more in depth demonstrations and `include/cassette/*.h` header files for full functions descriptions. For more information about the language usage, features and syntax check out the [language spec](./doc/spec.md).

Fizz Buzz
---------

Here's a Fizz Buzz example that prints the results to stderr using DEBUG_PRINT sequences. It's important to note that although it is possible, CCFG is not intended to be used for computation. Instead, all language features and functions have been created with dynamic configurations in mind.

```
LET_ENUM n 1 100
FOR_EACH n

	SECTION_DEL Fizz Buzz No_Fizz No_Buzz

	SECTION_ADD (== (MOD ((% n) 3) 0 Fizz No_Fizz)
	SECTION_ADD (== (MOD ((% n) 5) 0 Buzz No_Buzz)

	SECTION Fizz Buzz
		DEBUG_PRINT "Fizz Buzz"
	SECTION Fizz No_Buzz
		DEBUG_PRINT "Fizz"
	SECTION No_Fizz Buzz
		DEBUG_PRINT "Buzz"
	SECTION No_Fizz No_Buzz
		DEBUG_PRINT (% n)
	SECTION

FOR_END
```

Fuzzing <a name="fuzzing"></a>
-------

This project comes with an integrated fuzz test case. First make sure to have AFL++ and the necessary utilities (afl-gcc-fast) installed. Then build and run it with the following command:

```
make fuzzer
```

By default, CCFG will run without restrictions; thus, some language functions can generate hangs during fuzzing. CCFG provides a restricted mode ([Section 2.5](./doc/spec.md)). In restricted mode, only resources definitions are valid, and all language functions are disabled. To run the fuzz test case in restricted mode, run the following command:

```
CCFG_RESTRICT= make test
```

Mirrors
-------

- https://github.com/fraawlen/cassette-configuration
- https://codeberg.org/fraawlen/cassette-configuration
