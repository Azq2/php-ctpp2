--TEST--
CTPP load/unload, parse/save
--FILE--
<?php
	error_reporting(E_ALL);
	
	// new
	$T = new CTPP2();
	if (is_object($T)) {
		echo "new CTPP2: OK\n";
	} else {
		exit("new CTPP2: ERROR\n");
	}
	
	$methods = "__construct setIncludeDirs parseTemplate parseText loadBytecode loadBytecodeString saveBytecode ".
		"dumpBytecode freeBytecode param reset json output display dump getLastError bind unbind";
	foreach (explode(" ", $methods) as $m) {
		if (!method_exists($T, $m))
			exit("CTPP2 method exists ($m): ERROR\n");
	}
	echo "CTPP2 methods: OK\n";
	
	// parseText
	$b = $T->parseText("<TMPL_var text>");
	if (is_resource($b)) {
		echo "CTPP2::parseText: OK\n";
	} else {
		exit("CTPP2::parseText: ERROR\n");
	}
	
	// parse
	$b = $T->parseTemplate("examples/hello.tmpl");
	if (is_resource($b)) {
		echo "CTPP2::parseTemplate: OK\n";
	} else {
		exit("CTPP2::parseTemplate: ERROR\n");
	}
	
	// save
	if ($T->saveBytecode($b, "hello.ct2") && filesize("hello.ct2") > 5) {
		echo "CTPP2::saveBytecode: OK\n";
	} else {
		exit("CTPP2::saveBytecode: ERROR\n");
	}
	
	// load
	$b = $T->loadBytecode("hello.ct2");
	if (is_resource($b)) {
		echo "CTPP2::loadBytecode: OK\n";
	} else {
		exit("CTPP2::loadBytecode: ERROR\n");
	}
	
	// free
	$T->freeBytecode($b);
	echo "CTPP2::freeBytecode: OK\n";
	
	$b = $T->parseTemplate("examples/hello.tmpl");
	$bytecode = $T->dumpBytecode($b);
	if (is_string($bytecode) && strlen($bytecode) > 5) {
		echo "CTPP2::dumpBytecode: OK\n";
	} else {
		exit("CTPP2::dumpBytecode: ERROR\n");
	}
	
	$T->freeBytecode($b);
	
	$b = $T->loadBytecodeString($bytecode);
	if (is_resource($b)) {
		echo "CTPP2::loadBytecodeString: OK\n";
	} else {
		exit("CTPP2::loadBytecodeString: ERROR\n");
	}
	$T->freeBytecode($b);
?>
--EXPECT--
new CTPP2: OK
CTPP2 methods: OK
CTPP2::parseText: OK
CTPP2::parseTemplate: OK
CTPP2::saveBytecode: OK
CTPP2::loadBytecode: OK
CTPP2::freeBytecode: OK
CTPP2::dumpBytecode: OK
CTPP2::loadBytecodeString: OK