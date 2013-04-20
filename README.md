Rdbn
====
Fast code to learn deep belief networks in R.

Rdbn was written to allow training and classification using restricted Boltzmann machines (RBMs) and deep belief networks (DBNs) in an R environment.  

Rdbn supports:
* Pre-training a deep belief network using ideas from 'contrastive divergence'.
* Fine-tuning the network for classification tasks using the backpropagation algorithm.
* Advanced training features such as momentum-accelerated learning, and L2 regularization.
* Parallel processing on UNIX-based systems using pthreads.


Installing
----------
In a UNIX based OS with R and dev tools installed, one can simply type: 

  R CMD install Rdbn/

R should take care of the rest.  Note that on OS X developer tools must be installed in order to have a C compiler.  Windows is not supported at present.  This package is intended for eventual submission to CRAN, and Windows support will most likely be added at that time.


Using the package
-----------------
See the examples in the Rdbn/test_functions/ folder for insight on how to interact with the package in R.  Vignette and reference manual coming soon!


Useful References
-----------------
Nice Science paper demonstrating some useful applications:
http://www.cs.toronto.edu/~hinton/science.pdf

General overview on Scholarpedia (by Hinton):
http://www.scholarpedia.org/article/Boltzmann_machine
http://www.scholarpedia.org/article/Deep_belief_networks

Some excellent, abridged notes on the 'dark art' parts of the method:
http://www.cs.toronto.edu/~hinton/absps/guideTR.pdf

Hinton's Coursea course on neural networks:
https://class.coursera.org/neuralnets-2012-001/lecture/index

Several additional references listed in specific sections of the source code.

Copyright/ Licence
==================

Copyright 2013 Charles Danko

The enclosed script files are free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or  (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
