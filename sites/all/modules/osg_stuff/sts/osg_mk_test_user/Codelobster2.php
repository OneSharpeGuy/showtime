<pre>
<?php
$terms = array (
    'f' =>
    array (
        0 =>
        array (
            'term' => 'Soprano',
        ),
        1 =>
        array (
            'term' => 'Alto',
        ),
    ),
    'm' =>
    array (
        0 =>
        array (
            'term' => 'Tenor',
        ),
        1 =>
        array (
            'term' => 'Bass',
        ),
    ),
);


foreach (glob("*.txt") as $filename) {
    $mode = substr($filename,0,1);
    /* If the first letter of the file begins with

    M assume male names
    F assume female namespace
    anything else, assume gender neutral
    */
    if (array_key_exists($mode, $terms)) {
        $voice_parts = $terms[$mode];
    }else
    foreach ($terms as $item) {
        $voice_parts[] = $item;
    }
    $flat_file = file($filename, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    foreach ($flat_file as $line) {
        $peeps[] = array(
            'name'=>ucfirst(strtolower($line))
            ,'eligible_parts'=>$voice_parts);
    }


}
var_export($peeps);
?>
