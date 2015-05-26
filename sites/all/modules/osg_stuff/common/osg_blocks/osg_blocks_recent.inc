<?php
/**
 * @author Charles Sharpe
 * @copyright 2013
 */
/**
 * Logic Block for osg_blocks_recent
 *
 */
global $block_title, $user;
$block_title = 'Content viewed today';
//$node_types=variable_get('osg_blocks_recent_nodetypes', array())
$supress_hidden = (boolean)variable_get('osg_blocks_recent_supress_hidden', false);
$supress_innacessible = (boolean)variable_get('osg_blocks_recent_supress_inaccessible', false);
if ($supress_innacessible) {
    $sql = 'SELECT r.rid,r.name AS role_name ';
    $sql = $sql . 'FROM {users_roles} s ';
    $sql = $sql . 'INNER JOIN {role} r ON r.rid=s.rid ';
    $sql = $sql . 'WHERE uid=:uid; ';
    $user_roles = array_flip(db_query($sql, array(":uid" => $user->uid))->fetchAllKeyed());
}
$sql_format = 1;
if ($supress_hidden) {
    $sql_format = $sql_format + 2;
}
if ($supress_innacessible) {
    $sql_format = $sql_format + 4;
}
//$sql_format = 5;
switch ($sql_format) {
        /*
        Bitwise SQL Format
        1  node_counter  (Always on)
        2  suppress hidden
        4  suppress inaccessible
        */
    case 1:
        $sql = "SELECT n.nid,daycount,`timestamp`,title, '' AS settings ";
        $sql = $sql . "FROM node_counter c ";
        $sql = $sql . "INNER JOIN node n ON c.nid=n.nid ";
        $sql = $sql . "WHERE n.`status`=1 ";
        break;
    case 3:
        $sql = 'SELECT n.nid,daycount,`timestamp`,title, \'\' AS settings ';
        $sql = $sql . 'FROM node_counter c ';
        $sql = $sql . 'INNER JOIN node n ON c.nid=n.nid ';
        $sql = $sql . 'INNER JOIN hidden_nodes h ON c.nid=h.nid AND hidden=0 ';
        break;
    case 5:
        $sql = 'SELECT n.nid,daycount,`timestamp`,title, COALESCE(settings,\'\') settings,n.uid ';
        $sql = $sql . 'FROM node_counter c ';
        $sql = $sql . 'INNER JOIN node n ON c.nid=n.nid ';
        $sql = $sql . 'LEFT JOIN content_access a ON c.nid=a.nid ';
        $sql = $sql . 'WHERE n.`status`=1 ';
        break;
    case 7:
        $sql = 'SELECT n.nid,daycount,`timestamp`,title, COALESCE(settings,\'\') settings,n.uid ';
        $sql = $sql . 'FROM node_counter c ';
        $sql = $sql . 'INNER JOIN node n ON c.nid=n.nid ';
        $sql = $sql . 'INNER JOIN hidden_nodes h ON c.nid=h.nid AND hidden=0 ';
        $sql = $sql . 'LEFT JOIN content_access a ON c.nid=a.nid ';
        $sql = $sql . 'WHERE n.`status`=1 ';
        break;
}
$node_types = variable_get('osg_blocks_recent_nodetypes', array());
if (count($node_types) > 0) {
    $comma = '';
    $clause = " AND n.`type` IN(";
    foreach ($node_types as $key => $value) {
        if ($key = $value) {
            $clause = $clause . "$comma '$value'";
            $comma = ',';
        }
    }
    $clause = $clause . ")";
}
$sql = $sql . $clause;
$estimate = variable_get('osg_blocks_recent_nodecount', 10);
//$estimate = 999;
if ($estimate == 0) {
    $sql = $sql . ' AND daycount>0 ';
}
//$sql = $sql . " and n.nid=139 ";
$sql = $sql . " ORDER BY `timestamp` DESC";
$sql = $sql . ";";
//debug($sql);
drupal_set_message($sql);
$result = db_query($sql);
$rowcount = $result->rowCount();
$timestamp = time() + 86400;
$addendum = '';
if ($rowcount > 0) {
    $heap = variable_get('osg_blocks_recent_span_schema', '');
    if ($heap == '') {
        $heap = osg_blocks_recent_span_schema_init();
    }
    $rows = $result->fetchAll();
    $bottom = count($heap) - 1;
    $marktime = time();
    $i = count($rows) - 1;
    if ($estimate == 0) {
        $sql = 'SELECT COUNT(*) ' . substr($sql, strpos(strtolower($sql), 'from '));
        $estimate = db_query($sql)->fetchField();
    }
    if ($i > ($estimate - 1)) {
        $i = ($estimate - 1);
    }
    $breaker = $rows[$i]->timestamp;
    $target = ($marktime - $breaker) / 3600;
    if ($bottom > 0) {
        for ($i = 0; $i <= ($bottom); $i++) {
            if ($heap[$i]['max'] >= $target) {
                $breaker = $marktime - ($heap[$i]['max'] * 3600);
                $addendum = 'Viewed in the last ' . $heap[$i]['label'];
                break;
            }
        }
        for ($j = 0; $j < ($rowcount); $j++) {
            if ($rows[$j]->timestamp < $breaker) {
                break;
            }
            $show_node = true;
            $settings = unserialize($rows[$j]->settings);
            if (is_array($settings)) {
                //debug($settings);
                $show_node = false;
                foreach (array('view', 'view_own') as $key) {
                    foreach ($user_roles as $needle) {
                        //debug($needle, '$needle');
                        //debug($settings[$key], 'haystack');
                        if (in_array($needle, $settings[$key])) {
                            $show_node = true;
                            break;
                        }
                    }
                    if ($show_node) {
                        break;
                    }
                    if ($rows[$j]->uid <> $user->uid) {
                        break;
                    }
                }
            }
            if ($show_node) {
                $items[] = $rows[$j];
            }
        }
        //debug($items);
        $body = '<div class="item-list"><ul>';
        foreach ($items as $row) {
            $hits = $row->daycount;
            switch ($hits) {
                case 1;
                    $hits = "$hits view";
                    break;
                case 0;
                    $hits = "";
                    break;
                default;
                    $hits = "$hits views";
            }
            if ($hits == '') {
                $tipper = "Last viewed " . date("l, F jS", $row->timestamp) . " at " . date('h:i:s A', $row->timestamp);
            }
            else {
                $tipper = "$hits. Last view at " . date('h:i:s A', $row->timestamp);
            }
            $body = $body . "<li><a href=\"/node/" . $row->nid . "\" title=\"$tipper\">" . $row->title . "</a></li>";
        }
        $body = $body . "</ul></div>";
        $block_title = t($addendum);
    }
}
function osg_blocks_recent_span_schema_init() {
    $looper[] = array(
        'start' => 1,
        'end' => 5,
        'increment' => 1,
        'divisor' => 1,
        'term' => 'hour',
        );
    $looper[] = array(
        'start' => 6,
        'end' => 18,
        'increment' => 6,
        'divisor' => 1,
        'term' => 'hour',
        );
    $looper[] = array(
        'start' => 24,
        'end' => 48,
        'increment' => 12,
        'divisor' => 1,
        'term' => 'hour');
    $looper[] = array(
        'start' => 72,
        'end' => 168,
        'increment' => 24,
        'divisor' => 24,
        'term' => 'day');
    $looper[] = array(
        'start' => 336,
        'end' => 672,
        'increment' => 168,
        'divisor' => 168,
        'term' => 'week');
    $looper[] = array(
        'start' => 1344,
        'end' => 8568,
        'increment' => 672,
        'divisor' => 168,
        'term' => 'week');
    $looper[] = array(
        'start' => 8736,
        'end' => 43680,
        'increment' => 8736,
        'divisor' => 8736,
        'term' => 'year');
    foreach ($looper as $loop) {
        $starter = $loop['start'];
        $ender = $loop['end'];
        $incr = $loop['increment'];
        while ($starter <= $ender) {
            $counter = $starter / $loop['divisor'];
            if ($starter == 1) {
                $blurb = $loop['term'];
            }
            else {
                $blurb = $counter . ' ' . $loop['term'] . ($counter <> 1 ? 's' : '');
            }
            $heap[] = array('label' => $blurb, 'max' => $starter);
            $starter = $starter + $incr;
        }
    }
    variable_set('osg_blocks_recent_span_schema', $heap);
    debug($heap, 'osg_blocks_recent_span_schema', 'osg_blocks_recent_span_schema', true);
    return $heap;
}
