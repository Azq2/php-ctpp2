--TEST--
CTPP inner inclusion
--FILE--
<?php
	error_reporting(E_ERROR);
	
	// new
	$T = new CTPP2();
	$T->setIncludeDirs(array("examples", "./"));
	$b = $T->parseText("'<TMPL_include 'hello.tmpl'>'");
	$T->param(array(
		'world' => array(
			'name' => "World"
		)
	));
	
	if (base64_encode($T->output($b)) !== "J0hlbGxvLCBXb3JsZCEKCic=")
		exit("inner include: ERROR '".base64_encode($T->output($b))."'\n");
	echo "inner include: OK\n";
?>
--EXPECT--
inner include: OK
