<?php

/**
 * Auto Register Report
 */
function osg_auto_register_report($flags = array())
{

    if (!is_array($flags))
    {
        $hold = $flags;
        $flags = array();
        $flags[] = $hold;
    }
    if (!isset($flags['force']))
    {
        $flags['force'] = true;
    }
    if (osg_auto_register_report_needed())
    {

        $query = db_select('osg_auto_registration_request', 'q');
        $query->join('osg_users', 'u', 'q.uid=u.uid');
        $query->join('osg_ical_imported', 'i', 'q.nid = i.nid');
        $query->fields('q', array(
            'label',
            'nid',
            'uid'));
        $query->fields('u', array('user_name', 'voice_part'));
        $query->fields('i', array('title', 'start_time_unix'));
        $query->addField('q', 'inserted', 'notification_time');
        $query->addField('q', 'updated', 'response_time');
        $query->addExpression('q.updated-q.inserted', 'lag_time');
        $query->addExpression("CONCAT(u.first_name,' ',u.last_name)",
            'display_name');
        $query->isNotNull('q.updated');
        $cutoff = array(':cutoff' => variable_get('osg_auto_register_report_cutoff',
                7));
        $query->where('(FLOOR(UNIX_TIMESTAMP()/86400)- FLOOR(updated/86400))<=:cutoff',
            $cutoff);

        $query->orderBy('q.updated', 'DESC');
        $query->orderBy('i.start_time_unix', 'DESC');

        //        drupal_set_message((string )$query);

        $result = $query->execute();

        $fmt = "D M jS g:i a";
        /*
        foreach ($result as $row) {
        $columns = array();
        $columns['Response Time'] = date("F j, Y, g:i a",$row['updated']);
        );
        }
        */
        if ($result->rowCount())
        {


            $column = array();

            while ($record = $result->fetchAssoc())
            {

                $column = array();
                //$column['Notification Time'] = date($fmt, $record['notification_time']);
                $column['Response Time'] = date($fmt, $record['response_time']);
                $hrs = floor($record['lag_time'] / 3600);
                $mins = ($record['lag_time'] - $hrs * 3600) / 60;
                $secs = floor($mins - floor($mins));
                $lag_time = '';
                if ($hrs > 0)
                {
                    $lag_time = $hrs . _plurality(' hr', $hrs);
                }
                if ($mins > 0)
                {
                    $lag_time = $lag_time . ' ' . floor($mins) . _plurality(' min',
                        $mins);

                }
                if ($hrs < 1)
                {
                    if ($secs > 0)
                    {
                        $secs = $secs . _plurality(' secs', $secs);
                        if ($lag_time > '')
                        {
                            $lag_time = $lag_time . ' ' . $secs;
                        }
                        else
                        {
                            $lag_time = $secs;
                        }
                        $lag_time;
                    }
                }
                $column['Response Delay'] = array(
                    'data' => $lag_time,
                    'align' => 'right',
                    );
                $column["Member's Name"] = $record['display_name'];
                $column['Event Title'] = $record['title'];
                $column['Event Date'] = date($fmt, $record['start_time_unix']);
                $column['Response'] = $record['label'];

                $rows[] = array('data' => $column, 'class' => array('table-row'));

            }
            /*
            debug($column);
            debug($rows);
            debug($column);
            debug($rows);
            */
            $data = array(
                'header' => array_keys($column),
                'rows' => $rows,
                'attributes' => array(
                    'border' => 1,
                    'cellspacing' => 1,
                    'cellpadding' => 5,
                    ),
                );
            $table = theme('table', $data);
        }
        //debug($table);
        osg_auto_register_prep_message($table);
        //drupal_goto(' < front > ');

    }

}


function osg_auto_register_prep_message($info)
{
    global $base_url;
    //debug($data,'$data');
    $key = 'osg_auto_register';
    $message = array();
    //$separator = md5(time());
    // carriage return type (we use a PHP end of line constant)
    //$eol = PHP_EOL;
    //debug($info);return;
    ////$params['to'] = $record['email'];
    $send_mail = true;

    $recipient = variable_get('osg_auto_register_recipeint',
        'onesharpeguy@gmail.com');

    drupal_set_message("Mail sent to " . $recipient);
    $sender = variable_get('site_mail', 'admin@example.com');

    $message['subject'] = variable_get('osg_auto_register_email_subject',
        'Auto Register Email');
    $body = variable_get('osg_auto_register_email_message', $info);

    $body = token_replace($body);

    $message['body'][] = $body;
    //$message['headers']['Content - Type'] = 'text / html; charset = UTF - 8; format = flowed; delsp = yes';
    //drupal_mail($module,$key,$to,$language,$params = array(),$from = NULL,$send = TRUE)

    drupal_mail('osg_auto_register', 'osg_auto_register', $recipient,
        language_default(), $message, $sender, $send_mail);
    //debug(array($recipient,$sender,$send_mail));
}


function osg_auto_register_mail($key, &$message, $params)
{
    global $user;
    $options = array('langcode' => $message['language']->language, );


    // Each message is associated with a language, which may or may not be the
    // current user's selected language, depending on the type of e - mail being
    // sent. This $options array is used later in the t() calls for subject
    // and body to ensure the proper translation takes effect.
    if (($key == 'osg_auto_register') || ($key == 'osg_auto_register_pen'))
    {

        $options = array('langcode' => $message['language']->language, );
        $message['body'] = $params['body'];
        $message['subject'] = check_plain($params['subject']);
        //$message['headers']['Content - Type'] = $params['headers']['Content - Type'] ;
        $bcc = variable_get('osg_auto_register_bcc');
        if (is_array($bcc))
        {
            if (count($bcc) > 0)
            {
                $message['headers']['Bcc'] = implode(',', $bcc);
            }
        }
    }
}


function osg_auto_register_config_form($form, &$form_state)
{


    $form['markup'] = array('#markup' => t('Define parameters for auto registration email report'), );
    $form['osg_auto_register_blast_report_enable'] = array(
        '#type' => 'checkbox',
        '#title' => t('Autosend AutoRegister Report'),
        '#desciption' => t('Autosend Report?'),
        '#default_value' => variable_get('osg_auto_register_blast_report_enable', true),
        );
    $form['osg_auto_register_blast_unregistered_subject'] = array(
        '#type' => 'textfield',
        '#title' => t('Subject'),
        '#required' => true,
        '#size' => 128,
        '#desciption' => t('Subject of non-respondant email blast unregistered'),
        '#default_value' => variable_get('osg_auto_register_blast_unregistered_subject',
            'Auto-registration Report'),
        '#states' => array('visible' => array(':input[name="osg_auto_register_blast_report_enable"]' =>
                    array('checked' => true), ), ),
        );
    $default = 'The following is a list of ShowTime Performances for which the specified uses have auto-registered. ';
    $form['osg_auto_register_blast_unregistered_message_before'] = array(
        '#type' => 'textarea',
        '#title' => t('Message (before listing)'),
        '#description' => t('Text of email (before list of unregistered performances).'),
        '#default_value' => variable_get('osg_auto_register_blast_unregistered_message_before',
            $default),
        '#required' => true,
        '#states' => array('visible' => array(':input[name="osg_auto_register_blast_report_enable"]' =>
                    array('checked' => true), ), ),
        );
    $form['osg_auto_register_blast_unregistered_message_after'] = array(
        '#type' => 'textarea',
        '#title' => t('Addtional Message'),
        '#description' => t('Text of email (after the list of unregistered performances).'),
        '#default_value' => variable_get('osg_auto_register_blast_unregistered_message_after'),
        '#states' => array('visible' => array(':input[name="osg_auto_register_blast_report_enable"]' =>
                    array('checked' => true), ), ),
        );
    $border = ' style="border:1px solid #202020;padding:1.5em;"';

    $form['osg_auto_register_notifier_enable'] = array(
        '#type' => 'checkbox',
        '#title' => t('Per Event Notifications'),
        '#desciption' => t('Send Per Event Notifications?'),
        '#default_value' => variable_get('osg_auto_register_notifier_enable', true),
        );
    $form['osg_auto_register_notifier_addresses'] = array(
        '#type' => 'textfield',
        '#title' => t('Email recipients'),
        '#default_value' => implode(',', variable_get('osg_auto_register_notifier_addresses',
            array())),
        '#description' => t('Comma seperated list of Notification recipients.'),
        '#size' => 120,
        '#states' => array('visible' => array(':input[name="osg_auto_register_notifier_enable"]' =>
                    array('checked' => true), ), ),
        );
    $form['configuration']['osg_auto_register_report_now'] = array(
        '#type' => 'button',
        '#value' => t('Report Now'),
        '#weight' => 50,
        //'#executes_submit_callback'=>false,
        '#submit' => array('osg_auto_register_report'),
        );
    $form['#validate'][] = 'osg_auto_register_config_expand_email_fields';
    return system_settings_form($form);
}
function _plurality($term, $value)
{
    if (floor($value) <> 1)
    {
        $term = $term . 's';
    }
    return $term;
}


/**
 * The form to provide a link to cron.php.
 */
function osg_auto_register_cron_form($form, &$form_state)
{
    $form['status'] = array(
        '#type' => 'fieldset',
        '#title' => t('Cron status information'),
        );
    $form['status']['intro'] = array('#markup' => '<div>' . t('The cron example demonstrates hook_cron() and hook_cron_queue_info() processing. If you have administrative privileges you can run cron from this page and see the results.') .
            '</div>', );
    $form['status']['last'] = array('#markup' => '<div>' . t('osg_auto_register_cron() will next execute the first time cron runs after %time (%seconds seconds from now)' .
            '</div>', array(
            '%time' => date_iso8601(variable_get('osg_auto_register_next_execution',
                time())),
            '%seconds' => variable_get('osg_auto_register_next_execution', time
                ()) - time(),
            )), );
    if (user_access('administer site configuration'))
    {
        $form['cron_run'] = array(
            '#type' => 'fieldset',
            '#title' => t('Run cron manually'),
            );
        $form['cron_run']['cron_reset'] = array(
            '#type' => 'checkbox',
            '#title' => t("Run osg_auto_register's cron regardless of whether interval has expired."),
            '#default_value' => false,
            );
        $form['cron_run']['cron_trigger'] = array(
            '#type' => 'submit',
            '#value' => t('Run cron now'),
            '#submit' => array('osg_auto_register_cron'),
            );
    }

    $form['configuration'] = array(
        '#type' => 'fieldset',
        '#title' => t('Configuration of osg_auto_register_cron()'),
        );
    $options = array(
        'control' => 'hours|1|24|6;days;weeks|1|1;',
        'cron_safe' => true,
        'base' => 'hours');
    $form['configuration']['osg_auto_register_interval'] = array(
        '#type' => 'select',
        '#title' => t('Cron interval'),
        '#description' => t('Time after which osg_auto_register_cron will respond to a processing request.'),
        '#default_value' => variable_get('osg_auto_register_interval', 60 * 60),
        '#options' => osg_api_time_dropdown($options),
        );
    return system_settings_form($form);
}

function osg_auto_register_cron()
{
    if (variable_get('osg_auto_register_blast_report_enable', true))
    {
        $interval = variable_get('osg_auto_register_interval', 300);
        if ((!is_null($interval)) && is_numeric($interval) && ($interval > 0))
        {
            drupal_set_message('osg_auto_register_cron');
            // We usually don't want to act every time cron runs (which could be every
            // minute) so keep a time for the next run in a variable.
            //variable_del('osg_auto_register_next_execution');
            $next_exec = (int)variable_get('osg_auto_register_next_execution', 0);
            if (time() >= $next_exec)
            {

                osg_auto_register_report();
                variable_set('osg_auto_register_next_execution', time() + $interval);
            }
            else
            {
                $message = 'Next execution ' . $next_exec . ' Time Now ' . time() .
                    ' ' . (string )($next_exec - time());
                debug($message);
            }
        }
    }
}
function osg_auto_register_report_needed()
{
    osg_auto_register_housekeeping();
    $needed = false;
    $query = db_select('osg_auto_registration_request', 'q');
    $query->addField('q', 'updated');
    $query->orderBy('updated', 'DESC');
    $query->range(0, 1);
    $mark_time = $query->execute()->fetchField();
    if (is_null($mark_time))
    {
        $mark_time = time();
    }
    if ($mark_time > variable_get('osg_auto_register_last_report_time', 0))
    {
        variable_set('osg_auto_register_last_report_time', $mark_time);
        $needed = true;
    }
    return $needed;
}

function osg_auto_register_notification($authenticator, $testing = null)
{
    global $base_url;
    $message = array();
    //$separator = md5(time());
    // carriage return type (we use a PHP end of line constant)
    //$eol = PHP_EOL;

    ////$params['to'] = $record['email'];
    $send_mail = true; //(bool) (strpos($info['mail'],'@example.com') == 0);


    $peeps = variable_get('osg_auto_register_notifier_addresses', array());

    if (count($peeps))
    {
        $recipient = array_shift($peeps);
        if (count($peeps))
        {
            $message['headers']['Cc'] = implode(',', $peeps);
        }
    }


    //drupal_set_message("Mail sent to " . $recipient);
    $sender = variable_get('site_mail', 'admin@example.com');
    $split = explode('@', $sender);
    $message['headers']['Reply-To'] = 'no-reply@' . $split[1];
    $message['subject'] = token_replace(variable_get('osg_auto_register_mail_subject',
        '[site:name] -- Auto Registration'));

    /*
    SELECT u.first_name,
    u.last_name,
    i.title,
    i.start_time_unix,
    a.label,
    a.inserted,
    a.updated,
    COALESCE(j.last_notification_time, j.initial_notification_time) notification_time,
    j.notification_count
    FROM osg_auto_registration_request a
    INNER JOIN osg_ical_imported i ON a.nid = i.nid
    INNER JOIN osg_users u ON a.uid = u.uid
    INNER JOIN osg_singout_notifier_journal j ON a.uid = j.uid AND a.nid = j.nid
    WHERE a.authenticator='54cb26fa-mah6yzru8udpv5dc4dz6dr7rgulfvk4g'
    */

    $query = db_select('osg_auto_registration_request', 'a');
    $query->join('osg_ical_imported', 'i', 'a.nid = i.nid');
    $query->join('osg_users', 'u', 'a.uid = u.uid');
    $query->where('authenticator=:authenticator', array(':authenticator' => $authenticator));
    $query->fields('a', array(
        'label',
        'inserted',
        'updated',
        'nid'));
    $query->fields('i', array('title', 'start_time_unix'));
    $query->fields('u', array('first_name', 'last_name'));

    $query->join('osg_singout_notifier_journal', 'j',
        'a.uid = j.uid AND a.nid = j.nid');
    $query->addExpression('COALESCE(j.last_notification_time, j.initial_notification_time)',
        'notification_time');
    $query->addField('j', 'notification_count');


    $result = $query->execute();
    $row = $result->fetchAssoc();

    /*
    array (
    'label'          => 'Yes',
    'inserted'       => '1390983194',
    'updated'        => '1391002052',
    'title'          => 'Brighton Gardens of Columbia',
    'start_time_unix'=> '1392490800',
    'first_name'     => 'Cheryl',
    'last_name'      => 'Sharpe',
    )a
    */

    if (is_null($row['updated']))
    {
        $row['updated'] = time();
    }
    $row['elapsed'] = _ago($row['updated'] - $row['notification_time']);
    $row['notification_time'] = date('g:i a \o\n l, F jS Y', $row['notification_time']);
    if ($row['notification_count'] == 1)
    {
        $row['notification_count'] = 'One (1) notification was sent at';
        $adverbage = '';
    }
    else
    {
        $row['notification_count'] = _number_words($row['notification_count']) .
            ' notifications were sent, the last at';
        $adverbage = 'most recent';
    }
    $row['inserted'] = date('l, F jS Y', $row['inserted']);
    $row['start_time_unix'] = date('l, F jS Y', $row['start_time_unix']);
    $row['updated'] = date('g:i a \o\n l, F jS Y', $row['updated']);
    //debug($testing);
    $message['subject'] = $message['subject'] . " -- " . $row['title'];
    if ($testing)
    {
        $message['subject'] = '** TEST ** ' . $message['subject'];
    }
    $title_link = '<a href="' . $base_url . '/node/' . $row['nid'] .
        '/osg/singout/detail">' . $row['title'] . '</a>';
    $body = $row['first_name'] . ' ' . $row['last_name'] .
        " has used the AutoRegistration feature to respond <strong>" .
        strtoupper($row['label']) . "</strong> to <i>" . $title_link . ' (' . $row['start_time_unix'] .
        ')</i>. ' .$row['notification_count'] . ' ' . $row['notification_time'] .
        '. This registration occured at ' . $row['updated'] . ', ' . $row['elapsed'] .
        "after the $adverbage notification was sent.";

    $message['body']['html'] = $body;
    $message['headers']['Content-Type'] =
        'text/html; charset=UTF-8; format=flowed; delsp=yes';
    //debug($message,'$message');
    //drupal_mail($module,$key,$to,$language,$params = array(),$from = NULL,$send = TRUE)


    drupal_mail('osg_auto_register', 'osg_auto_register_pen', $recipient,
        language_default(), $message, $sender, $send_mail);
    //debug(array($recipient,$sender,$send_mail));

    //debug($message,'$message');
}
function _number_words($value)
{
    if ($value < 10)
    {
        switch ($value)
        {
            case 1:
                $value = "One (1)";
                break;
            case 2:
                $value = "Two (2)";
                break;
            case 3:
                $value = "Three (3)";
                break;
            case 4:
                $value = "Four (4)";
                break;
            case 5:
                $value = "Five (5)";
                break;
            case 6:
                $value = "Six (6)";
                break;
            case 7:
                $value = "Seven (7)";
                break;
            case 8:
                $value = "Eight (8)";
                break;
            case 9:
                $value = "Nine (9)";

        }
    }
    return $value;
}

function osg_auto_register_config_expand_email_fields($form, &$form_state)
{
    $emails = $form_state['values']['osg_auto_register_notifier_addresses'];

    $emails = str_replace(';', ',', $emails);
    $emails = str_replace('|', ',', $emails);
    $emails = explode(',', $emails);

    form_set_value($form['osg_auto_register_notifier_addresses'], $emails, $form_state);


}
function _ago($time)
{
    $periods = array(
        "second",
        "minute",
        "hour",
        "day",
        /*  "week",
        "month",
        "year",
        "decade"
        */
        );
    $lengths = array(
        "60",
        "60",
        "24",
        "7",
        /*
        "4.35",
        "12",
        "10"
        */
        );

    $now = time();

    $difference = $time;
    $tense = "ago";

    for ($j = 0; $difference >= $lengths[$j] * 3 && $j < count($lengths) - 1; $j++)
    {
        $difference /= $lengths[$j];
    }

    $difference = round($difference);

    if ($difference != 1)
    {
        $periods[$j] .= "s";
    }

    return "$difference $periods[$j] ";
}
