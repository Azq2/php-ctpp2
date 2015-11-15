--TEST--
CTPP parse example templates and test errors reporting
--FILE--
<?php
	error_reporting(E_ERROR);
	
	// new
	$T = new CTPP2();
	$files = array(
		'examples/charset_recoder.tmpl',
		'examples/example1.tmpl',
		'examples/example10.tmpl',
		'examples/example11.tmpl',
		'examples/example2.tmpl',
		'examples/example3-1.tmpl',
		'examples/example3-2.tmpl',
		'examples/example3-3.tmpl',
		'examples/example3.tmpl',
		'examples/example4.tmpl',
		'examples/example5.tmpl',
		'examples/example5_gettext.tmpl',
		'examples/example5_iconv.tmpl',
		'examples/example5_md5.tmpl',
		'examples/example6.tmpl',
		'examples/example7.tmpl',
		'examples/example8.tmpl',
		'examples/example9.tmpl',
		'examples/footer.tmpl',
		'examples/header.tmpl',
		'examples/hello.tmpl',
		'examples/logic-error.tmpl',
		'examples/math_expr.tmpl',
		'examples/middle.tmpl',
		'examples/runtime-error.tmpl',
		'examples/syntax-error.tmpl',
		'examples/transmap.tmpl'
	);
	
	foreach ($files as $file) {
		$b = $T->parseTemplate($file);
		if ($b) {
			$T->saveBytecode($b, "$file.c2");
			$T->freeBytecode($b);
			
			$b = $T->loadBytecode("$file.c2");
			if ($b) {
				echo "$file: OK\n";
			} else {
				printf("loadBytecode(%s.ct2): ERROR 0x%08X (%s) at line %d, pos %d\n", $file, $err["error_code"], $err["error_str"], $err["line"], $err["pos"]);
			}
			// $T->output($b);
		} else {
			$err = $T->getLastError();
			printf("%s: ERROR 0x%08X (%s) at line %d, pos %d\n", $file, $err["error_code"], $err["error_str"], $err["line"], $err["pos"]);
		}
	}
?>
--EXPECT--
examples/charset_recoder.tmpl: OK
examples/example1.tmpl: OK
examples/example10.tmpl: OK
examples/example11.tmpl: ERROR 0x04000011 (unknown block name) at line 1, pos 52
examples/example2.tmpl: ERROR 0x04000011 (incorrect include file name) at line 6, pos 38
WARNING: near line 1, pos. 16: comparison result of INTEGER VALUE is always false
WARNING: near line 2, pos. 16: comparison result of INTEGER VALUE is always true
WARNING: near line 3, pos. 18: comparison result of FLOAT VALUE is always true
WARNING: near line 4, pos. 18: comparison result of FLOAT VALUE is always true
WARNING: near line 5, pos. 20: comparison result of STRING VALUE is always true
WARNING: near line 6, pos. 17: comparison result of STRING VALUE is always true
WARNING: near line 8, pos. 16: comparison result of INTEGER VALUE is always false
WARNING: near line 10, pos. 13: comparison result of INTEGER VALUE is always true
examples/example3-1.tmpl: OK
examples/example3-2.tmpl: OK
examples/example3-3.tmpl: OK
WARNING: near line 1, pos. 12: comparison result of INTEGER VALUE is always true
WARNING: near line 2, pos. 12: comparison result of INTEGER VALUE is always false
WARNING: near line 3, pos. 14: comparison result of FLOAT VALUE is always true
WARNING: near line 4, pos. 14: comparison result of FLOAT VALUE is always true
WARNING: near line 5, pos. 16: comparison result of STRING VALUE is always true
WARNING: near line 6, pos. 13: comparison result of STRING VALUE is always true
WARNING: near line 8, pos. 12: comparison result of INTEGER VALUE is always true
WARNING: near line 10, pos. 13: comparison result of INTEGER VALUE is always true
examples/example3.tmpl: OK
examples/example4.tmpl: ERROR 0x04000011 (incorrect operator) at line 1, pos 7
examples/example5.tmpl: OK
examples/example5_gettext.tmpl: OK
examples/example5_iconv.tmpl: OK
examples/example5_md5.tmpl: OK
examples/example6.tmpl: ERROR 0x04000011 (expected '>') at line 3, pos 31
examples/example7.tmpl: ERROR 0x04000011 (incorrect include file name) at line 4, pos 15
examples/example8.tmpl: OK
WARNING: near line 2, pos. 12: comparison result of INTEGER VALUE is always false
examples/example9.tmpl: ERROR 0x04000011 (incorrect operator) at line 25, pos 7
examples/footer.tmpl: OK
examples/header.tmpl: OK
examples/hello.tmpl: OK
examples/logic-error.tmpl: OK
examples/math_expr.tmpl: OK
examples/middle.tmpl: OK
examples/runtime-error.tmpl: OK
examples/syntax-error.tmpl: ERROR 0x04000011 (expected at least one space symbol) at line 2, pos 14
examples/transmap.tmpl: ERROR 0x04000011 (incorrect operator) at line 4, pos 7
