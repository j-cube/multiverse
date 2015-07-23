# multiverse
Next generation data storage back-end for the widely used Alembic file format

## organization

The original upstream Alembic code resides in the [master](https://github.com/j-cube/multiverse) branch.

The `multiverse` code, for `1.5.8` release, resides in its own [1.5.8/multiverse](https://github.com/j-cube/multiverse/tree/1.5.8/multiverse) branch
(this branch could be merged and even fast-forwarded on the upstream `1.5.8` tag).

To use `multiverse`, just checkout its branch:

```
$ git clone https://github.com/j-cube/multiverse.git
...

$ git checkout 1.5.8/multiverse
```

## multiverse dependencies

In addition to the original Alembic dependencies, `multiverse` needs:

* [libgit2 v0.22.1](https://github.com/libgit2/libgit2/archive/v0.22.1.tar.gz)
* [msgpack 1.0.1](https://github.com/msgpack/msgpack-c/releases/download/cpp-1.0.1/msgpack-1.0.1.tar.gz)
* [boost 1.48.0](http://sourceforge.net/projects/boost/files/boost/1.48.0/boost_1_48_0.tar.bz2/download) compiled also with `chrono`, `filesystem`, `system` (in addition to the original `program_options`, `thread` and `python`)

## multiverse license

`multiverse` modifications and additions are:

```
multiverse - a next generation storage back-end for Alembic
Copyright (C) 2015 J CUBE Inc. Tokyo, Japan. All Rights Reserved.     
                                                                     
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.          
                                                                     
J CUBE Inc.                                                              
6F Azabu Green Terrace                                                   
3-20-1 Minami-Azabu, Minato-ku, Tokyo                                    
info@-jcube.jp                                                           

```

See the [LICENSE-multiverse.txt](LICENSE-multiverse.txt) file for details.

The rest of Alembic remains covered by the original [LICENSE.txt](LICENSE.txt) file.

## original README

See also the original [README.txt](README.txt).
