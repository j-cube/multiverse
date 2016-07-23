# multiverse
Next generation data storage back-end for the widely used Alembic file format

## organization

The original upstream Alembic code resides in the [master](https://github.com/j-cube/multiverse) branch.

The `multiverse` code for `2.0.0` release, resides in its own [2.0.0/multiverse](https://github.com/j-cube/multiverse/tree/2.0.0/multiverse) branch.

To use `multiverse`, just checkout its branch:

```
$ git clone https://github.com/j-cube/multiverse.git
...

$ git checkout 2.0.0/multiverse
```

Note that `multiverse` code for `1.5.8` and `2.0.0` releases match Alembic 1.5.8 and these branch could be merged and even fast-forwarded on the upstream `1.5.8` tag.

## multiverse dependencies

In addition to the original Alembic dependencies, `multiverse` needs:

* [libgit2 v0.23.4](https://github.com/libgit2/libgit2/archive/v0.23.4.tar.gz)
* [msgpack 1.0.1](https://github.com/msgpack/msgpack-c/releases/download/cpp-1.0.1/msgpack-1.0.1.tar.gz)
* [boost 1.48.0](http://sourceforge.net/projects/boost/files/boost/1.48.0/boost_1_48_0.tar.bz2/download) compiled also with `chrono`, `filesystem`, `system` (in addition to the original `program_options`, `thread` and `python`)

## multiverse license

`multiverse` modifications and additions are:

```
multiverse - a next generation storage back-end for Alembic

Copyright 2015 J CUBE Inc. Tokyo, Japan.     
                                                                     
Licensed under the Apache License, Version 2.0 (the "License");         
you may not use this file except in compliance with the License.        
You may obtain a copy of the License at                                 
                                                                        
    http://www.apache.org/licenses/LICENSE-2.0                          
                                                                        
Unless required by applicable law or agreed to in writing, software     
distributed under the License is distributed on an "AS IS" BASIS,       
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and     
limitations under the License.                             

```
         
```
Contact Informations:

    J CUBE Inc.                                                          
    6F Azabu Green Terrace                                                   
    3-20-1 Minami-Azabu, Minato-ku, Tokyo, Japan                                 
    info@-jcube.jp                                                           
    http://j-cube.jp
```

See the [LICENSE-multiverse.txt](LICENSE-multiverse.txt) file for details.

The rest of Alembic remains covered by the original [LICENSE.txt](LICENSE.txt) file.

## original README

See also the original [README.txt](README.txt).
