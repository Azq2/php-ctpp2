--TEST--
CTPP recoder test
--FILE--
<?php
	error_reporting(E_ERROR);
	
	// new
	$T = new CTPP2();
	$b = $T->parseTemplate("examples/charset_recoder.tmpl");
	$T->param(array(
		'a' => iconv("UTF8", "CP1251", 'Тест кодировки ')
	));
	if (base64_encode($T->output($b)) !== "0uXx8jog0uXx8iDq7uTo8O7i6uggCg==")
		exit("charset_recoder: ERROR '".base64_encode($T->output($b))."'\n");
	echo "charset_recoder: OK\n";
	
	if (base64_encode($T->output($b, "CP1251", "UTF-8")) !== "0KLQtdGB0YI6INCi0LXRgdGCINC60L7QtNC40YDQvtCy0LrQuCAK")
		exit("charset_recoder2: ERROR '".base64_encode($T->output($b, "CP1251", "UTF-8"))."'\n");
	echo "charset_recoder2: OK\n";
	
	$T = new CTPP2(array(
		charset_dst => "UTF-8", 
		charset_src => "CP1251"
	));
	$T->param(array(
		'a' => iconv("UTF8", "CP1251", 'Тест кодировки CP1251')
	));
	$b = $T->parseTemplate("examples/charset_recoder.tmpl");
	
	if (base64_encode($T->output($b)) !== "0uXx8jog0uXx8iDq7uTo8O7i6uggQ1AxMjUxCg==")
		exit("charset_recoder3: ERROR '".base64_encode($T->output($b))."'\n");
	echo "charset_recoder3: OK\n";
?>
--EXPECT--
charset_recoder: OK
charset_recoder2: OK
charset_recoder3: OK
