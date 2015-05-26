<pre><?php

  $fudge = array(
        '5 minutes' => 300,
        '30 minutes' => 1800,
        '1 hour' => 3600,
        '6 hours' => 21600,
        '12 hours' => 43200,
        '1 day' => 86400,
    );
	$val=86400;
	$word="day";
	$s="";
	$options=array();
	for($i = 1; $i <6 ; $i++){
		$options["$i $word$s"]=$val*$i;
	}
	var_export($options);
?>