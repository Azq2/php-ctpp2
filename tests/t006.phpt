--TEST--
CTPP syscalls
--FILE--
<?php
	error_reporting(E_ERROR);
	
	$T = new CTPP2();
	$T->setIncludeDirs(array("examples", "./"));
	$b = $T->parseText("'<TMPL_var OLOLO('XUJ', 3.14, HTMLESCAPE('<>'))>'");
	if (!$T->output($b)) {
		$err = $T->getLastError();
		$errstr = sprintf("ERROR 0x%08X (%s) at line %d, pos %d", $err["error_code"], $err["error_str"], $err["line"], $err["pos"]);
		if ($errstr === 'ERROR 0x0200000C (Unsupported syscall: "OLOLO") at line 0, pos 0')
			echo "invalid_syscall_test: OK\n";
		else
			exit("invalid_syscall_test: ERROR ($errstr)\n");
	} else {
		exit("invalid_syscall_test: ERROR\n");
	}
	
	$T->bind("OLOLO", function ($a, $b, $c) {
		return "$a, $b, $c";
	});
	if ($T->output($b) !== "'XUJ, 3, &lt;&gt;'")
		exit("bind syscall: ERROR\n");
	echo "bind syscall: OK\n";
	
	$T->unbind("OLOLO");
	if ($T->output($b) !== false && ($err = $T->getLastError()) && $err['error_code'] === 0x0200000C)
		exit("unbind syscall: ERROR\n");
	echo "unbind syscall: OK\n";
?>
--EXPECT--
invalid_syscall_test: OK
bind syscall: OK
unbind syscall: OK
