<?php
//Connect To Database
$sts_hostname='50.63.244.154';
$sts_username='osgshowtime';
$sts_dbname='osgshowtime';
$con=mysql_connect($sts_hostname,$sts_username, 'WHAreel7.10');

if (!$con)
{
	die('Could not connect: ' . mysql_error());
}

mysql_select_db($sts_dbname);
?>