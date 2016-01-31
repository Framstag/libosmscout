libosmscout can partially be used from Java. For this SWIG is used
to scan the C++ header files and generate a C++ JNDI stub library and
some java classes on top of it.

The following sub directories are involved:

libosmscout-binding:
  autoconf project that starts SWIG to generate the C++ stub code and 
  the resulting Java classes. It also builds the stub library:
  
  Use
    ./autogen.sh && ./configure 
  to build the Java code and the C++ libary. You must have SWIG in the
  search path and a JDK (for jni.h) in the search path. Make sure the
  generated library is in your library search path (like all other 
  libosmscout libraries).
  
  Use
    mvn install  
  to build the libsosmcout.jar and import it to your local maven repository.
  
  
Java:
  a Java/Maven project with some partial examples that show how much
  works.  
