# Multiverse

`Multiverse` is the next-generation data storage back-end for the widely used [Alembic](https://github.com/alembic/alembic) file format.

> Authors: Marco Pantaleoni, Aghiles Kheffache, Bo Zhou, Paolo Berto Durante.


The _"Multiverse"_ name is also used for the proprietary commercial product [Multiverse | Studio](http://multi-verse.io). `Multiverse | Studio` is a fast, efficient and easy to use solution for CGI/VFX studios, whose main purposes are to scale complexity in look-development, render procedurally, inter-op between DCC Apps like [Maya](https://autodesk.com/products/maya) and [Katana](https://foundry.com/products/katana). `Multiverse | Studio` offers full integration for the Multiverse back-end and as of June 2017 is distributed by [Foundry](https://foundry.com).

> NOTE: This repository is about the Multiverse _back-end_, not the end-user product _Multiverse | Studio.


## Repository organization

The original upstream Alembic code resides in the [master](https://github.com/j-cube/multiverse/tree/master) branch.

In terms of version numbering, `Multiverse` stays in-sync with the Alembic releases: 

the `Multiverse` code for the `1.7.1` release, resides in its own [1.7.1/multiverse](https://github.com/j-cube/multiverse/tree/1.7.1/multiverse) branch and is the default landing branch for the repository when browsing on GitHub.

To use `Multiverse`, just checkout its branch:

```
$ git clone https://github.com/j-cube/multiverse.git
...

$ git checkout 1.7.1/multiverse
```


## Multiverse dependencies

In addition to the original Alembic dependencies, `Multiverse` needs the following additional dependencies:

* [libgit2 v0.23.4+](https://github.com/libgit2/libgit2/archive/v0.23.4.tar.gz)
* [msgpack 1.0.1+](https://github.com/msgpack/msgpack-c/releases/download/cpp-1.0.1/msgpack-1.0.1.tar.gz)
* [boost 1.48.0+](http://sourceforge.net/projects/boost/files/boost/1.48.0/boost_1_48_0.tar.bz2/download) compiled also with `chrono`, `filesystem`, `system` (in addition to the original `program_options`, `thread` and `python`)

### "Classic" Git and Milliways

`Multiverse` source contains [Milliways](https://github.com/j-cube/milliways) "The storage at the back-end of the Multiverse". Milliways is a high-performance on-disk tree-based key-value store, used in Multiverse as an optional pluggable back-end to `libgit2`. `Milliways` was created to improve performance over the "classic" Multiverse Git backend by introducing a single file store database. As of release 1.7.1 we default to Milliways and a single .abc file is created, making the Git repository non visible to the user. CLassic Git repository can always be created optionall. 


## Research

`Multiverse` and `Milliways` are research projects by J CUBE Inc. Multiverse was published at SIGGRAPH Asia 2015 in Kobe, Japan. MOre information on [J CUBE Research](http://j-cube.jp/research).

## Multiverse license

`Multiverse` modifications and additions are:

```
Multiverse - a next generation storage back-end for Alembic

Copyright 2015—2017 J CUBE Inc. Tokyo, Japan.     
                                                                     
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
Authors:

 Marco Pantaleoni
 Aghiles Kheffache
 Bo Zhou
 Paolo Berto Durante
```

```
Contact Informations:

    J CUBE Inc.                                                          
    6F Azabu Green Terrace                                                   
    3-20-1 Minami-Azabu, Minato-ku, Tokyo, Japan                                 
    info@-jcube.jp                                                           
    http://j-cube.jp
```

See the [LICENSE-multiverse.txt](LICENSE-multiverse.txt) file for more details.

The rest of Alembic remains covered by the original [LICENSE.txt](LICENSE.txt) file.

See also the original [README.txt](README.txt) part of Alembic.
