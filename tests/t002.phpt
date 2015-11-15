--TEST--
CTPP setIncludeDirs, set/dump params
--FILE--
<?php
	error_reporting(E_ALL);
	
	// new
	$T = new CTPP2();
	$T->setIncludeDirs(array(
		".", "examples"
	));
	
	$b = $T->parseTemplate("examples/hello.tmpl");
	if (!is_resource($T->parseTemplate("examples/hello.tmpl")))
		exit("CTPP2::setIncludeDirs: ERROR\n");
	$T->saveBytecode($b, "hello.ct2");
	
	if (!is_resource($T->parseTemplate("hello.tmpl")))
		exit("CTPP2::setIncludeDirs: ERROR\n");
	echo "CTPP2::setIncludeDirs: OK\n";
	
	$b = $T->loadBytecode("hello.ct2");
	$T->param(array(
		"world" => array(
			"name" => "beautiful World"
		)
	));
	
	class A {
		public function __toString() {
			return __METHOD__;
		}
	}
	
	if ($T->output($b) !== "Hello, beautiful World!\n\n") {
		exit("CTPP2::param: ERROR\n");
	}
	echo "CTPP2::param: OK\n";
	
	$T->reset();
	if ($T->output($b) !== "Hello, !\n\n") {
		exit("CTPP2::reset: ERROR\n");
	}
	echo "CTPP2::reset: OK\n";
	
	$T->param(array(
		"world" => array(
			"name" => "ololo World"
		), 
		"lalala" => function () {
			return "!!!!";
		}, 
		"1" => new A, 
		fopen("/etc/hosts", "r"), 
		2 => 3.1456455645, 
		"test" => array(
			1, 2, 3
		), 
		"rewrite" => 0
	));
	$T->param(array(
		"test1" => array(
			3 => 1, 2, 3, "a" => 43
		), 
		"test2" => array(
			0 => 1, 35
		), 
		"test3" => array(
			1 => 1, 35
		), 
		"test4" => array(
			0 => 1, 2 => 35
		), 
		"rewrite" => 1
	));
	
	if (base64_encode($T->dump()) !== "ewogICcxJyA6ICJBOjpfX3RvU3RyaW5nIiwKICAnMicgOiAzLjE0NTY0NTU2NDUsCiAgJ2xhbGFsYScgOiAiISEhISIsCiAgJ3Jld3JpdGUnIDogMSwKICAndGVzdCcgOiBbCiAgICAgICAgICAgICAxLAogICAgICAgICAgICAgMiwKICAgICAgICAgICAgIDMKICAgICAgICAgICBdLAogICd0ZXN0MScgOiB7CiAgICAgICAgICAgICAgJzMnIDogMSwKICAgICAgICAgICAgICAnNCcgOiAyLAogICAgICAgICAgICAgICc1JyA6IDMsCiAgICAgICAgICAgICAgJ2EnIDogNDMKICAgICAgICAgICAgfSwKICAndGVzdDInIDogWwogICAgICAgICAgICAgIDEsCiAgICAgICAgICAgICAgMzUKICAgICAgICAgICAgXSwKICAndGVzdDMnIDogewogICAgICAgICAgICAgICcxJyA6IDEsCiAgICAgICAgICAgICAgJzInIDogMzUKICAgICAgICAgICAgfSwKICAndGVzdDQnIDogewogICAgICAgICAgICAgICcwJyA6ICwKICAgICAgICAgICAgICAnMicgOiAzNQogICAgICAgICAgICB9LAogICd3b3JsZCcgOiB7CiAgICAgICAgICAgICAgJ25hbWUnIDogIm9sb2xvIFdvcmxkIgogICAgICAgICAgICB9Cn0=") {
		exit("CTPP2::dump: ERROR\n");
	}
	echo "CTPP2::dump: OK\n";
	
	if ($T->output($b) !== "Hello, ololo World!\n\n") {
		exit("CTPP2::output: ERROR\n");
	}
	echo "CTPP2::output: OK\n";
	
	$T->param(array("world" => array("name" => "World")));
	if ($T->output($b) !== "Hello, World!\n\n") {
		exit("CTPP2::param 2: ERROR\n");
	}
	echo "CTPP2::param 2: OK\n";
	
	$T->json('{"world": {"name": "Ololo"}}');
	if ($T->output($b) !== "Hello, Ololo!\n\n") {
		exit("CTPP2::json: ERROR\n");
	}
	echo "CTPP2::json: OK\n";
?>
--EXPECT--
CTPP2::setIncludeDirs: OK
CTPP2::param: OK
CTPP2::reset: OK
CTPP2::dump: OK
CTPP2::output: OK
CTPP2::param 2: OK
CTPP2::json: OK
