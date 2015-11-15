PHP_ARG_ENABLE(ctpp2, for ctpp2, 
	[AS_HELP_STRING([--enable-ctpp2], [enable ctpp2 (default is no)])])

if test "${PHP_CTPP2}" != "no"; then
	if test -r $PHP_CTPP2/include/ctpp2/CDT.hpp; then
		CTPP_INCLUDE_DIR=$PHP_CTPP2/include/ctpp2
		CTPP_LIB_DIR=$PHP_CTPP2/lib
	else
		SEARCH_INCLUDE_PATH="/usr/local/include/ctpp2 /usr/include/ctpp2 /opt/incude/ctpp2"
		AC_MSG_CHECKING([for ctpp header files in default path])
		for i in $SEARCH_INCLUDE_PATH ; do
			if test -r $i/CDT.hpp; then
				CTPP_INCLUDE_DIR=$i
				AC_MSG_RESULT(found in $i)
			fi
		done
		
		SEARCH_LIB_PATH="/usr/local/lib /usr/lib /opt/lib /usr/local/lib64 /usr/lib64 /opt/lib64"
		AC_MSG_CHECKING([for ctpp library in default path])
		for i in $SEARCH_LIB_PATH ; do
			if test -r $i/libctpp2.so; then
				CTPP_LIB_DIR=$i
				AC_MSG_RESULT(found in $i)
			fi
		done
	fi
	
	if test -z "$CTPP_INCLUDE_DIR"; then
		AC_MSG_ERROR([CTPP2 headers not found. ])
	fi
	
	if test -z "$CTPP_LIB_DIR"; then
		AC_MSG_ERROR([CTPP2 libs not found. ])
	fi
	
	PHP_ADD_INCLUDE($CTPP_INCLUDE_DIR)
	PHP_ADD_LIBRARY_WITH_PATH("ctpp2", $CTPP_LIB_DIR, CTPP2_SHARED_LIBADD)
	
	PHP_SUBST(CTPP2_SHARED_LIBADD)
	CXXFLAGS="-std=c++11"
	PHP_REQUIRE_CXX()
	PHP_ADD_LIBRARY(stdc++, 1, CTPP2_SHARED_LIBADD)
	PHP_NEW_EXTENSION(ctpp2, src/php_ctpp2.cpp src/ctpp2.cpp src/ctpp2_functions.cpp, $ext_shared)
fi
