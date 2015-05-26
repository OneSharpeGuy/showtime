<?php

/**
 * @author Charles Sharpe
 * @copyright 2013
 */

/**
 *
 * @param array $options
 *
 * @return
 */
function osg_singout_sync_parse_calendar_feed($options = array())
{
    osg_singout_sync_sync_db_columns_with_schema();
    osg_singout_sync_housekeeping();
    $options['session_uuid'] = osg_api_GUID();
    $options['span'] = osg_singout_sync_availability();
    $calendars = variable_get('osg_singout_sync_calendar', array());
    //debug($calendars);
    foreach ($calendars as $item)
    {
        $options['url'] = $item['url'];
        $_SESSION['osg_singout_sync_calendar'] = $item;
        $_SESSION['osg_singout_sync_touch_time'] = REQUEST_TIME;
        osg_singout_sync_parse_single_url($options);
    }

    osg_singout_support_reset_table_osg_registration_ext();
}
function osg_singout_sync_parse_single_url($options = array())
{

    $session_uuid = $options['session_uuid'];
    $span = $options['span'];

    $url = $options['url'];
    //debug($span);
    //debug($url);
    if (($url == '') || (is_null($url)))
    {

        drupal_set_message(t('Must define <a href="@config">Google Calendar URL</a>.',
            array('@config' => url('admin/config/system/osg_singout_sync'))),
            'warning');
    } else
    {
        $flat_file = file($url, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
        $flat_file = osg_singout_sync_normalize_vcard($flat_file);

        //$flat_file = preg_replace("'([\r\n])[\s] + '", '', $flat_file);
        //Read the flat file line by line
        $ical = array();
        if (!isset($events))
        {
            $events = array();
        }

        $date_mask = 'l, F j, Y h:ia';
        foreach ($flat_file as $line)
        {
            //new line characters are escaped as \n
            $escapers = array(
                '\,' => ',',
                '\n' => '<br>',
                );
            //iCal format escapes commas.
            foreach ($escapers as $key => $value)
            {
                $line = trim(str_replace($key, $value, $line));
            }

            //$fields = explode(":", $line);
            $fields = osg_singout_sync_controlled_explosion(":", $line);
            //var_export($fields);

            switch ($line)
            {
                    // http://www.kanzaki.com / docs / ical / vtodo.html
                case "BEGIN:VCALENDAR":
                    $calendar = array();
                    $mode = "calendar";
                    break;
                case "BEGIN:VEVENT":
                    $event = array();
                    break;

                case "END:VCALENDAR":
                    $calendar[] = $events;
                    $ical[] = $calendar;
                    break;
                case "END:VEVENT":

                    if (isset($event['start_time']))
                    {
                        if (time() - $event['start_time']['unix_timestamp'] <
                            604800) //more than one week ago
                        {
                            $event['time_zone'] = $calendar['time_zone'];
                            foreach ($event as $key => $value)
                            {

                            }
                            $event['available'] = 0;
                            if ($event['start_time']['unix_timestamp'] >= $span['start'])
                            {
                                if ($event['start_time']['unix_timestamp'] <= $span['end'])
                                {
                                    $event['available'] = 1;
                                }
                            }


                            $events[] = $event;
                        }
                    }
                    break;
            }

            switch ($fields[0])
            {
                case "X-WR-CALNAME":
                    $calendar['name'] = $fields[1];
                    debug($_SESSION['osg_singout_sync_calendar']);
                    drupal_set_message('Calendar Name: ' . $calendar['name']);

                    break;
                case "X-WR-TIMEZONE":
                    $calendar['time_zone'] = $fields[1];
                    date_default_timezone_set($fields[1]);
                    break;
                case "DTSTART":
                    $event['start_time'] = array('native' => trim($fields[1]),
                            'unix_timestamp' => strtotime($fields[1]));
                    $event['start_time']['display'] = date($date_mask, $event['start_time']['unix_timestamp']);
                    break;
                case "DTEND":
                    $event['end_time'] = array('native' => trim($fields[1]),
                            'unix_timestamp' => strtotime($fields[1]));
                    $event['end_time']['display'] = date($date_mask, $event['end_time']['unix_timestamp']);
                    break;
                case "DESCRIPTION":
                    $event['description'] = $fields[1];
                    break;
                case "LOCATION":
                    $event['location'] = $fields[1];
                    break;
                case "SEQUENCE":
                    $event['sequence'] = $fields[1];
                    break;
                case "UID":
                    $event['unique_id'] = $fields[1];
                    break;
                case "SUMMARY":
                    $event['title'] = $fields[1];
                    break;
            }


        }


        $pivot = array();
        foreach ($events as $event)
        {
            $pivot[] = &$event['start_time']['unix_timestamp'];
        }
        array_multisort($pivot, $events);
        foreach ($events as $event)
        {
            drupal_set_message('Checking ' . $event['title']);
            $status = osg_singout_sync_event_status($event, $session_uuid);
            if ($event['available'])
            {
                //debug($status['changes']);
            }
            if (isset($status['mode']))
            {
                if ($status['mode'] == 'new')
                {
                    $node = osg_singout_sync_add_node($event, $span);
                } elseif ($status['mode'] == 'edit')
                {
                    $node = osg_singout_sync_edit_node($event, $span, $status, $options);
                }
                osg_singout_sync_push_touchtime($event['unique_id']);
                osg_singout_sync_check_ownership($node);
                _check_calendar_url($node->nid);
            }
        }

    }
}


function osg_singout_sync_date_compare($a, $b)
{
    $t1 = $a['start_time']['unix_timestamp'];
    $t2 = $b['start_time']['unix_timestamp'];
    if ($t1 == $t2)
    {
        $value = 0;
    } else
    {
        if ($t1 < $t2)
        {
            $value = -1;

        } else
        {
            $value = 1;
        }
    }
    return $value;
}
function osg_singout_sync_add_node($event, $span)
{
    global $user;
    $node = new stdClass();

    $node->type = "singout";
    node_object_prepare($node); // Sets some defaults. Invokes hook_prepare() and hook_node_prepare().
    $node->uid = $user->uid;
    $node->status = 1; //(1 or 0): published or not
    $node->promote = 0; //(1 or 0): promoted to front page
    $node->comment = 0; // 0 = comments disabled, 1 = read only, 2 = read / write

    $node->title = $event['title'];

    $node->field_venue = osg_api_push_fat_value($event['title']);
    $node->language = LANGUAGE_NONE;
    //$node->body = array('en' => array());
    $offset = explode(':', date('P', $event['start_time']['unix_timestamp']));

    $offset = $offset[0] * 3600 + $offset[1] * 60 * ($offset[0] < 0) ? -1 : 1;
    //    $node->field_when = array('und' => array(0 => array('value' => $event['start_time']['unix_timestamp'], ), ), );
    //    $node->field_lead_time = array('und' => array(0 => array('value' => 60, ), ), );
    //    $node->field_location = array('und' => array(0 => array('value' => $event['location'], ), ), );
    //    $node->field_registration_type = array('und' => array(0 => array('registration_type' =>
    //                'singout',),),);
    //    $node->format = 'full_html';
    //    $node->field_icaluid = array('und' => array(0 => array('value' => $event['unique_id'], ), ), );
    //    $node->field_description = array('und' => array(0 => array('value' => $event['description'], ), ), );
    $node->field_when = osg_api_push_fat_value($event['start_time']['unix_timestamp']);
    $node->field_lead_time = osg_api_push_fat_value(variable_get('osg_singout_sync_default_lead_time',
        60));
    $node->field_location = osg_api_push_fat_value($event['location']);
    $node->field_registration_type = osg_api_push_fat_value(array('registration_type' =>
            'singout'));
    $node->format = 'full_html';
    $node->field_icaluid = osg_api_push_fat_value($event['unique_id']);
    $node->field_description = osg_api_push_fat_value($event['description']);
    /*
    $node->field_available = array(
    'und' => array(
    0 => array(
    'value' =>
    ($event['start_time']['unix_timestamp'] > $span['start'])
    && ($event['start_time']['unix_timestamp'] < $span['end']),),),);
    */
    //$node->field_available = osg_api_push_fat_value($event['available']);
    $location = osg_api_get_flat_field('node', $node, 'field_location');

    osg_singout_sync_build_body($node, $event);
    $node = node_submit($node); // Prepare node for saving
    node_save($node);
    $fields['nid'] = $node->nid;

    if ($event['available'])
    {
        $fields['available'] = $event['available'];
    }
    $result = db_update('osg_ical_imported')->fields($fields)->condition('unique_id',
        $event['unique_id'])->execute();

    drupal_set_message($node->title . ' added');
    return $node;
}
/*
$event['start_time']['native'] = "20090607T160000Z"
$event['start_time']['unix_timestamp'] = "1244390400"
$event['start_time']['display'] = "Sunday, June 7, 2009 12:00pm"
$event['end_time']['native'] = "20090607T200000Z"
$event['end_time']['unix_timestamp'] = "1244404800"
$event['end_time']['display'] = "Sunday, June 7, 2009 04:00pm"
$event['unique_id'] = "nr8au81cfj1hb46g08scc6u0co@google.com"
$event['description'] = ""
$event['location'] = "Elicot City Senior Center"
$event['sequence'] = "2"
$event['title'] = "Annual ECSC Concert"

*/
//  $node = osg_singout_sync_edit_node($event,$span, $nid, $options);
function osg_singout_sync_edit_node($event, $span, $status, $options = array())
{
    //global $user;

    if (!array_key_exists('force', $options))
    {
        $options['force'] = false;
    }

    $node = node_load($status['values']['nid']);
    //$hold_node = node_load($nid);
    //debug($node);
    $node->type = "singout";
    $node->status = 1; //(1 or 0): published or not
    $node->comment = 0; // 0 = comments disabled, 1 = read only, 2 = read / write


    $node->format = 'full_html';
    $node->language = LANGUAGE_NONE;

    //$node->field_venue = array('und' => array(0 => array('value' => $event['title'], ), ), );
    //$node->field_when = array('und' => array(0 => array('value' => $event['start_time']['unix_timestamp'], ), ), );
    //$node->field_location = array('und' => array(0 => array('value' => $event['location'])));
    //$node->field_registration_type = array('und' => array(0 => array('registration_type' =>'singout')));
    //$node->field_description = array('und' => array(0 => array('value' => $event['description'], ), ), );
    $node->field_venue = osg_api_push_fat_value($event['title']);
    $node->field_when = osg_api_push_fat_value($event['start_time']['unix_timestamp']);
    $node->field_location = osg_api_push_fat_value($event['location']);
    //$node->field_registration_type = 'singout';
    $node->field_registration_type = osg_api_push_fat_value(array('registration_type' =>
            'singout'));
    $node->field_description = osg_api_push_fat_value($event['description']);
    //debug($event,'$event');
    //debug($span,'$span');
    //return $node;
    //This will determine whether the node is available or not, so write it regardless
    $value = 0;
    if ($event['start_time']['unix_timestamp'] > $span['start'])
    {
        if ($event['start_time']['unix_timestamp'] < $span['end'])
        {
            $value = 1;
        }
    }
    $node->field_available = osg_api_push_fat_value($event['available']);
    //This probably will not propagate, I just use it to update the ogc_ical_log
    //I'm relatively sure I don't need it anymore.


    $changed = $status['changes'];


    if (count($changed) || $options['force'])
    {
        osg_singout_sync_build_body($node, $event);
        $node = node_submit($node); // Prepare node for saving
        node_save($node);

        drupal_set_message('Updating ' . $node->title);

    }
    if (count($changed))
    {
        $msg = "The following properties have changed:<br>";
        foreach ($changed as $key => $value)
        {
            $msg = $msg . $key . "<br>";
        }
        osg_singout_sync_change_log($status);
        drupal_set_message($msg);
    }
    return $node;
}
/**
 * Calendar id from atom feed looks like 'http://www.google.com/calendar/feeds/stsingers%40gmail.com/public/basic/n5a6a22ojuiiicv7t26qv4dm14',
 * The last section should be good enough for our purposes so split it up, flip it, and take the top element
 *
 * */

function osg_singout_sync_parse_calendar_id($cal_id)
{

    $id_list = array_reverse(explode('/', $cal_id));

    return $id_list[0];
}


function osg_singout_summary_check_default_user()
{
    global $user;
    $default_user = $user;
    $test_user = user_load_by_name(variable_get('osg_singout_sync_default_user'));
    if (is_object($test_user))
    {
        if (property_exists($test_user, 'uid'))
        {
            $default_user = $test_user;
        }
    }

    return $default_user;
}

function osg_singout_sync_check_ownership($node)
{

    $uid = 0;

    if (property_exists($node, 'uid'))
    {
        $uid = $node->uid;
    }
    if ($uid <= 1)
    {
        $designee = variable_get('osg_singout_sync_default_user', null);
        if ($designee)
        {
            $sql = "UPDATE node n CROSS ";
            $sql = $sql . "JOIN ( ";
            $sql = $sql . "SELECT uid ";
            $sql = $sql . "FROM users ";
            $sql = $sql . "WHERE name=:designee ) u SET n.uid=u.uid ";
            $sql = $sql . "WHERE n.uid<=1 AND u.uid >1 ";
            db_query($sql, array(':designee' => $designee));

        }
    }
}
function osg_singout_sync_is_node_available($nid)
{
    $span = osg_singout_sync_availability();
    $record = db_select('osg_ical_imported', 'a')->fields('a')->condition('nid',
        $nid)->execute()->fetchAssoc();
    $available = ($span['start'] < $record['start_time_unix'] && $record['start_time_unix'] <
        $span['end']);
    //debug($record);
    //debug($span);
    //debug($available);
    return $available;
}

function osg_singout_sync_availability()
{
    $span = variable_get('osg_singout_sync_availability_span', array('pivot_date' =>
            array('start_of_tomorrow' => 0)));
    $span = array('pivot_date' => array('start_of_tomorrow' => 0));
    if (time() > $span['pivot_date']['start_of_tomorrow'])
    {
        $span['pivot_date'] = array('start_of_day' => mktime(0, 0, 0));
        $span['pivot_date']['caption'] = date('l, F jS Y', $span['pivot_date']['start_of_day']);
        $span['pivot_date']['start_of_tomorrow'] = $span['pivot_date']['start_of_day'] +
            86400;
        $span['pivot_date']['structure'] = getdate($span['pivot_date']['start_of_day']);
        $span['settings']['availability_start'] = variable_get('osg_singout_sync_availability_start',
            -1);
        $span['settings']['availability_end'] = variable_get('osg_singout_sync_availability_end',
            120);
        $span['settings']['promote'] = variable_get('osg_singout_sync_promote',
            7);
        $span['start'] = $span['pivot_date']['start_of_day'] + ($span['settings']['availability_start'] *
            86400);
        $span['end'] = $span['pivot_date']['start_of_day'] + ($span['settings']['availability_end'] *
            86400);
        $span['promote'] = $span['pivot_date']['start_of_day'] + ($span['settings']['promote'] *
            86400);


        foreach ($span as $key => $value)
        {
            if (gettype($value) == "integer")
            {
                $span['display_dates'][$key] = date('l jS \of F Y h:i:s A', $value);
            }

        }

        variable_set('osg_singout_sync_availability_span', $span);
    }

    return $span;
}
/**
 * VCARD format is 75 characters wide.  If new line is a continuation
 * of the previous line, first character is blank. This function stiches
 * continued lines to the previous line.
 *
 * @param array $dirty
 *
 */
function osg_singout_sync_normalize_vcard(array $dirty)
{
    $line = $dirty[0];
    foreach ($dirty as $item)
    {
        if (substr($item, 0, 1) == " ")
        {
            $line = $line . substr($item, 1, 80);

        } else
        {
            $better[] = $line;
            $line = $item;
        }

    }

    return $better;
}
function osg_singout_sync_endvalue($var)
{
    if (is_array($var))
    {
        $value = osg_singout_sync_endvalue($var);
    } else
    {
        $value = $var;
    }
    return $value;
}
function osg_singout_sync_changes($checklist, $old_node, $new_node)
{
    $changes = array();

    foreach ($checklist as $key)
    {
        //debug($old_node->$key,$key);
        //debug($new_node->$key,$key);
        $alpha = var_export($old_node->$key, true);
        $bravo = var_export($new_node->$key, true);

        if (!$alpha == $bravo)
        {
            if (substr($key, 0, 6) == 'field_')
            {
                //field_venue
                //012345

                $caption = substr($key, 6, strlen($key));
            } else
            {
                $caption = $key;
            }
            $changes[$caption] = true;
            //debug($changes,$caption);

        }
    }

}
function osg_singout_sync_change_log($status)
{

    //log changes
    $fields = array();
    //debug($status['changes']);
    foreach ($status['changes'] as $key => $item)
    {
        //debug($item);
        $sql = "INSERT INTO osg_ical_log ";
        $sql = $sql . "(unique_id ";
        $sql = $sql . ", session_uuid ";
        $sql = $sql . ", MESSAGE ";
        $sql = $sql . ", old_value ";
        $sql = $sql . ", new_value ";
        $sql = $sql . ", change_time) ";
        $sql = $sql .
            "VALUES     (:unique_id,:session_uuid,:message,:old_value,:new_value,:change_time) ";
        $values = array(
            'unique_id' => $status['values']['unique_id'],
            'session_uuid' => $status['session_uuid'],
            'message' => 'column [' . $key . '] changed',
            'old_value' => serialize($item['old']),
            'new_value' => serialize($item['new']),
            'change_time' => time(),
            );
        $result = db_query($sql, $values);
        //debug($values);
        $fields[$key] = $item['new'];
    }
    //update osg_ical_import
    $fields['last_update'] = time();
    //debug($fields,'$fields');
    $query = db_update('osg_ical_imported')->fields($fields)->condition('unique_id',
        $status['values']['unique_id']);
    //debug($query->__toString());
    $query->execute();
}
/**
 * Event Status
 * @param array $event
 * @param string $session_uuid
 *
 * $status['mode'] = "exists"
 * $status['values']['start_time_native'] = "20140622T180000Z"
 * $status['values']['start_time_unix'] = "1403460000"
 * $status['values']['session_uuid'] = ""
 * $status['values']['unique_id'] = "g44355jl2n5cg2s40cr70bk3oo@google.com"
 * $status['values']['location'] = "4475 Montgomery Rd Ellicott City, MD 21043
 * $status['values']['title'] = "Shangri-Ladida"
 * $status['values']['description'] = "Spring Concert"\
 * $status['values']['nid'] = "208"
 * $status['changes']['title']['old'] = "Shangri-Ladida"
 * $status['changes']['title']['new'] = "Shangri-La"
 * $status['key_map']['title'] = "field_venue"
 * $status['key_map']['start_time_unix'] = "field_when"
 * $status['key_map']['location'] = "field_location"
 * $status['key_map']['description'] = "field_description"

 *
 */
function osg_singout_sync_event_status($event, $session_uuid)
{
    $status = array();
    $status['session_uuid'] = $session_uuid;
    $sql = "SELECT g . * , f.entity_id AS nid ";
    $sql = $sql . "FROM osg_ical_imported g ";
    $sql = $sql . "INNER JOIN field_data_field_icaluid f ON g.unique_id = f.field_icaluid_value ";
    $sql = $sql . "AND f.bundle = 'singout' ";
    $sql = $sql . "AND f.entity_type = 'node' ";
    $sql = $sql . "WHERE g.unique_id = :unique_id ";
    $result = db_query($sql, array(':unique_id' => $event['unique_id']));
    //drupal_set_message($query->__toString());

    //$status['mode'] = 'unkown';
    if ($result->rowCount())
    {
        $record = $result->fetchAssoc();
        $status['mode'] = 'edit';
        $status['values'] = $record;
        $changes = array();
        //For comparison purposes, make NULL and '' equal
        foreach ($record as $key => $item)
        {
            if (is_null($item))
            {
                $record[$key] = '';
            }
        }
        if ((int)$record['start_time_native'] <> (int)$event['start_time']['native'])
        {
            $changes['start_time_native']['old'] = $record['start_time_native'];
            $changes['start_time_native']['new'] = $event['start_time']['native'];
            $changes['start_time_unix']['old'] = (int)$record['start_time_unix'];
            $changes['start_time_unix']['new'] = (int)$event['start_time']['unix_timestamp'];
        }
        if ($record['description'] <> $event['description'])
        {
            $changes['description']['old'] = $record['description'];
            $changes['description']['new'] = $event['description'];
        }
        if ($record['location'] <> $event['location'])
        {
            $changes['location']['old'] = $record['location'];
            $changes['location']['new'] = $event['location'];
        }
        if ($record['title'] <> $event['title'])
        {

            $changes['title']['old'] = $record['title'];
            $changes['title']['new'] = $event['title'];

        }
        if (count($changes) == 0)
        {
            $node = node_load($record['nid']);
            $fudge = osg_api_get_flat_field('node', $node, 'field_description');
            if ($event['description'] <> $fudge)
            {
                $changes['description']['old'] = $fudge;
                $changes['description']['new'] = $event['description'];
            }

            $fudge = osg_api_get_flat_field('node', $node, 'field_location');
            if ($event['location'] <> $fudge)
            {
                $changes['location']['old'] = $fudge;
                $changes['location']['new'] = $event['location'];
            }
            $fudge = osg_api_get_flat_field('node', $node, 'field_when');
            if ($event['start_time']['unix_timestamp'] <> $fudge)
            {
                $changes['start_time_unix']['old'] = (int)$fudge;
                $changes['start_time_unix']['new'] = (int)$event['start_time']['unix_timestamp'];
            }
        }
    } else
    {
        /*
        SELECT f.C
        FROM node n
        INNER JOIN field_data_field_icaluid f ON n.nid = f.entity_id AND f.entity_type='node'
        WHERE n.`type`='singout'
        */
        //        $query = db_select('node','n');
        //        $query->innerJoin('field_data_field_icaluid','f', "n.nid = f.entity_id AND f.entity_type = 'node'");
        //        $query->condition('n.type','singout');
        //        $query->condition('f.field_icaluid_value',$event['unique_id']);
        //        drupal_set_message($query->__toString());
        //        $query->fields('f',array('field_icaluid_value'));
        //        $result = $query->execute();
        $sql = "SELECT f.field_icaluid_value ";
        $sql = $sql . "FROM node n ";
        $sql = $sql .
            "INNER JOIN field_data_field_icaluid f ON n.nid = f.entity_id AND f.entity_type='node' ";
        $sql = $sql .
            "WHERE n.`type`='singout' AND f.field_icaluid_value=:unique_id ";
        $result = db_query($sql, array(':unique_id' => $event['unique_id']));
        if ($result->rowCount() == 0)
        {
            $query = db_insert('osg_ical_imported');
            $query->fields(array(
                'unique_id' => $event['unique_id'],
                'session_uuid' => $session_uuid,
                'start_time_native' => $event['start_time']['native'],
                'start_time_unix' => $event['start_time']['unix_timestamp'],
                'location' => $event['location'],
                'title' => $event['title'],
                'description' => $event['description'],
                'initial_import' => time(),
                'available' => $event['available'],
                'calendar_url' => $_SESSION['osg_singout_sync_calendar']['url'],
                ))->execute();

            $query = db_select('osg_ical_imported', 'o');
            $query->condition('unique_id', $event['unique_id']);
            $query->fields('o');
            $result = $query->execute();
            $record = $result->fetchAssoc();
            $status['mode'] = 'new';
            $status['values'] = $record;
            $changes = array();

        }
    }

    if ($record['available'] <> $event['available'])
    {

        $changes['available']['old'] = $record['available'];
        $changes['available']['new'] = $event['available'];
    }

    $status['key_map'] = array(
        'title' => 'field_venue',
        'start_time_unix' => 'field_when',
        'location' => 'field_location',
        'description' => 'field_description',
        'available' => 'field_available',
        );

    $status['changes'] = $changes;
    //debug($status,'$status');
    return $status;
}
/**
 * Cleanup know anomalies
 *
 */
function osg_singout_sync_housekeeping()
{


    //drupal_set_message('osg_singout_sync_housekeeping');
    // Delete hook has been added, but I leave this in place
    // to account for programming changes and such
    $sql = "DELETE ";
    $sql = $sql . "FROM osg_ical_imported ";
    $sql = $sql . "WHERE unique_id NOT IN ( ";
    $sql = $sql . "SELECT field_icaluid_value ";
    $sql = $sql . "FROM field_data_field_icaluid) ";
    db_query($sql);
    /*
    This section insures that already imported items neccessarily appear
    */
    $sql = "INSERT INTO osg_ical_imported ";
    $sql = $sql . "(unique_id ";
    $sql = $sql . ", session_uuid ";
    $sql = $sql . ", start_time_native ";
    $sql = $sql . ", start_time_unix ";
    $sql = $sql . ", title ";
    $sql = $sql . ", nid ";
    $sql = $sql . ", initial_import) ";
    $sql = $sql . " ";
    $sql = $sql . "SELECT u.field_icaluid_value ";
    $sql = $sql . ", s.session_uuid ";
    $sql = $sql . ", FROM_UNIXTIME(w.field_when_value,'%Y%m%dT%h%m%sZ') AS start_time_native ";
    $sql = $sql . ", w.field_when_value ";
    $sql = $sql . ", n.title ";
    $sql = $sql . ", n.nid ";
    $sql = $sql . ", UNIX_TIMESTAMP() ";
    $sql = $sql . "FROM   node n ";
    $sql = $sql . "INNER JOIN field_data_field_icaluid u ";
    $sql = $sql . "ON n.nid = u.entity_id ";
    $sql = $sql . "AND u.entity_type = 'node' ";
    $sql = $sql . "INNER JOIN field_data_field_when w ";
    $sql = $sql . "ON n.nid = w.entity_id ";
    $sql = $sql . "CROSS JOIN (SELECT UUID() AS session_uuid) s ";
    $sql = $sql . "LEFT JOIN osg_ical_imported z ";
    $sql = $sql . "ON u.field_icaluid_value = z.unique_id ";
    $sql = $sql . "WHERE  z.unique_id IS NULL ";
    db_query($sql);
    //Now fill in the nullable fields that may well be blank
    $sql = "UPDATE osg_ical_imported a ";
    $sql = $sql . "INNER JOIN field_data_field_location l ON a.nid = l.entity_id ";
    $sql = $sql . "AND l.entity_type = 'node' ";
    $sql = $sql . "SET location = l.field_location_value ";
    $sql = $sql . "WHERE a.location IS NULL ";
    db_query($sql);
    $sql = "UPDATE osg_ical_imported a ";
    $sql = $sql . "INNER JOIN field_data_field_description d ON a.nid = d.entity_id ";
    $sql = $sql . "AND d.entity_type = 'node' ";
    $sql = $sql . "SET a.description = d.field_description_value ";
    $sql = $sql . "WHERE a.description IS NULL ";
    db_query($sql);
    //The change time will still be null. We know what the current values are,
    // but we can't really say definitively if they have changed or when they changed

}
/*
function osg_singout_sync_column_sync()//not currently used
{
//$fields = osg_spammer_fields();
$full_schema = drupal_get_schema();
foreach($full_schema as $table_name => $schema)
{
if($schema['module'] == 'osg_singout_sync')
{
$fields = $schema['fields'];
//retrieve a record to ensure this schema matches what's in the database
$result = db_query("select * from $table_name LIMIT 1;")->fetchAssoc();
//debug($result, '$result', true);
//            foreach ($result as $column_name => $value) {
//                if (!array_key_exists($column_name, $fields)) {
//                    db_drop_field($table_name, $column_name);
//                    drupal_set_message($column_name . " dropped from table $table_name", 'status');
//                }
//            }
//debug($schema);
foreach($fields as $field_name => $field)
{
if($result)
{
if(!array_key_exists($field_name, $result))
{
unset($fields[$field_name]);
}
}
if(!array_key_exists($field_name, $fields))
{
db_add_field($table_name, $field_name, $field);
drupal_set_message($field_name . " added to table $table_name", 'status');
}
}
}
}
}
*/
function osg_singout_sync_controlled_explosion($del, $source)
{
    $fields = explode($del, $source);
    if (count($fields) > 2)
    {
        $data = array_slice($fields, 1, count($fields));

        $fields = array($fields[0], implode($del, $data));
    }

    return $fields;

}
function _check_calendar_url($nid)
{
    $url = db_select('osg_ical_imported', 'i')->fields('i', array('calendar_url'))->
        condition('nid', $nid)->execute()->fetchField();

    if ($url <> $_SESSION['osg_singout_sync_calendar']['url'])
    {
        db_update('osg_ical_imported')->fields(array('calendar_url' => $_SESSION['osg_singout_sync_calendar']['url']))->
            condition('nid', $nid)->execute();
    }

    //'calendar_url' => $_SESSION['osg_singout_sync_calendar']['url'],
}

/**
 * This function allows columns to be added to tables that have been defined in hook_schema
 *
 * @return
 */
function osg_singout_sync_sync_db_columns_with_schema()
{

    $full_schema = drupal_get_complete_schema(true);

    foreach ($full_schema as $table_name => $schema)
    {
        if ($schema['module'] == 'osg_singout_sync')
        {
            $fields = $schema['fields'];
            //retrieve a record to ensure this schema matches what's in the database
            $result = db_query("select * from $table_name LIMIT 1;")->
                fetchAssoc();

            if ($result)
            {

                foreach ($fields as $field_name => $field)
                {

                    if (!array_key_exists($field_name, $result))
                    {
                        db_add_field($table_name, $field_name, $field);
                        drupal_set_message($field_name . " added to table $table_name",
                            'status');
                    }
                }

            }
        }
    }
}
function osg_singout_sync_push_touchtime($unique_id)
{

    /*
    $num_updated = db_update('node') // Table name no longer needs {}
    ->fields(array(
    'uid' => 5,
    'status' => 1,
    ))
    ->condition('created', REQUEST_TIME - 3600, '>=')
    ->execute();
    */
    db_update('osg_ical_imported')->fields(array('last_seen' => REQUEST_TIME, ))->
        condition('unique_id', $unique_id)->execute();
}
function osg_singout_sync_disable_unseen()
{

    /*
    SELECT i.nid AS nid
    FROM osg_ical_imported i
    INNER JOIN node n ON i.nid=n.nid
    WHERE (n.`status`=1) AND ((last_seen < last_seen+1) OR (last_seen IS NULL))
    */
    $db_or = db_or();
    $db_or->condition('last_seen', $_SESSION['osg_singout_sync_touch_time'],
        ' < ');
    $db_or->isNull('last_seen');
    $query = db_select('osg_ical_imported', 'i')->fields('i', array('nid'));
    $query->join('node', 'n', 'i.nid=n.nid');
    $query->condition('n.`status`', 1);
    $query->condition($db_or);

    /*->where('(last_seen,<:touch_time) OR (last_seen IS NULL)'
    ,array(
    ':touch_time'=>$_SESSION['osg_singout_sync_touch_time']
    )
    );*/

    drupal_set_message((string )$query);
    drupal_set_message($_SESSION['osg_singout_sync_touch_time']);
    $result = $query->execute();

    foreach ($result as $record)
    {
        /*
        $query = db_update('node')
        ->fields(array('status'=>0))
        ->condition('nid',$record->nid);
        //drupal_set_message((string)$query);
        $query ->execute();
        */
        $node = node_load($record->nid);
        $node->status = 0;
        node_save($node);
        //debug($node);
        db_update('osg_ical_imported')->fields(array(
            'available' => 0,
            'last_update' => time(),
            ))->condition('nid', $record->nid)->execute();
        //debug($record);
    }

}
