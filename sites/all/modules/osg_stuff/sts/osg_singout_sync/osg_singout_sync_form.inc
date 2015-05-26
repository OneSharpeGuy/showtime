<?php

/**
 * The form to provide a link to cron.php.
 */
function osg_singout_sync_form($form, &$form_state)
{

    $cron_safe_threshold = variable_get('cron_safe_threshold');
    if ($cron_safe_threshold == 0)
    {
        $blurb = "Cron is not running.";
    }
    else
    {
        $next_cron_time = variable_get('cron_last', time()) + $cron_safe_threshold;

        $show_time = variable_get('osg_singout_sync_next_execution', time());
        if ($show_time < $next_cron_time)
        {
            $show_time = $next_cron_time;
        }
        $elap = $show_time - time();
        $blurb = 'STS singout sync cron job will execute at ' . date("Y-m-d H:i:s",
            $show_time);
        if ($elap > 0)
        {
            $blurb = $blurb . ' or in ' . $elap . ' seconds.';
        }
        else
        {
            $blurb = 'Will execute the next time cron runs.';
        }
    }
    $form['configuration']['exection_time'] = array(
        '#type' => 'markup',
        '#markup' => "<p>$blurb</p>",
        );
    $fudge = array(
        '5 minutes' => 300,
        '30 minutes' => 1800,
        '1 hour' => 3600,
        '6 hours' => 21600,
        '12 hours' => 43200,
        '1 day' => 86400,
        );
    $min = variable_get('cron_safe_threshold', 0);

    $options = array();
    foreach ($fudge as $key => $value)
    {
        if ($min <= $value)
        {
            $options[$value] = t($key);
        }
    }

    $options = array_merge(array(0 => t('Never')), $options);
    $form['configuration']['osg_singout_sync_interval'] = array(
        '#type' => 'select',
        '#title' => t('Cron interval'),
        '#description' => t('Time after which osg_singout_sync_cron will respond to a processing request.'),
        '#default_value' => variable_get('osg_singout_sync_interval', 60 * 60),
        '#options' => $options,
        '#disabled' => ($cron_safe_threshold == 0),
        );

    //debug($_SESSION);
    //*--------------------------------------------------------------------------------------
    //$form_state['storage']['osg_singout_sync_calendar'] =
    //isset($form_state['storage']['osg_singout_sync_calendar']) ? $form_state['storage']['osg_singout_sync_calendar'] : 1;
    //    dbug($_SESSION);

    $form['configuration']['osg_singout_sync_calendar'] = array(
        '#type' => 'container',
        '#tree' => true,
        '#prefix' => '<div id="container">',
        '#suffix' => '</div>',
        );
    $defaults = variable_get('osg_singout_sync_calendar', array());
    $defaults = osg_sinout_sync_squish_values($defaults);
    //debug($defaults);
    if (is_array($defaults))
    {
        $calendar_count = count($defaults);

    }
    if (!isset($calendar_count) || ($calendar_count < 0))
    {
        $calendar_count = 0;
    }
    if (isset($form_state['storage']['counter']))
    {
        $control_count = $form_state['storage']['counter'];
    }
    else
    {
        $control_count = 0;
    }

    //debug($calendar_count, '$calendar_count');
    //debug($control_count, '$control_count');
    if ($control_count > $calendar_count)
    {
        $calendar_count = $control_count;
    }

    if ($calendar_count < 1)
    {
        $calendar_count = 1;
    }
    if ($calendar_count > $control_count)
    {
        $form_state['storage']['counter'] = $calendar_count;
    }
    $roles = osg_singout_sync_get_roles();

    for ($i = 0; $i < $calendar_count; $i++)
    {
        $form['configuration']['osg_singout_sync_calendar'][$i] = array(
            '#type' => 'fieldset',
            '#tree' => true,
            '#title' => t('Google Calendar (# ' . ($i + 1) . ')'),
            );
        $form['configuration']['osg_singout_sync_calendar'][$i]['url'] = array(
            '#type' => 'textfield',
            '#title' => t('URL'),
            '#size' => 128,
            '#desciption' => t('Address of the Calendar from which to import'),
            '#default_value' => isset($defaults[$i]) ? $defaults[$i]['url'] : null,
            );
        //----
        $form['configuration']['osg_singout_sync_calendar'][$i]['date_label'] =
            array(
            '#type' => 'textfield',
            '#title' => t('Date Label'),
            '#size' => 128,
            '#desciption' => t('Display label for date.'),
            '#default_value' => isset($defaults[$i]['date_label']) ? $defaults[$i]['date_label'] :
                'Date',
            );
        $form['configuration']['osg_singout_sync_calendar'][$i]['start_time_label'] =
            array(
            '#type' => 'textfield',
            '#title' => t('Start Time Label'),
            '#size' => 128,
            '#desciption' => t('Display label for starting time.'),
            '#default_value' => isset($defaults[$i]['start_time_label']) ? $defaults[$i]['start_time_label'] :
                'Start Time',
            );
        /* $form['configuration']['osg_singout_sync_calendar'][$i]['display_derrived_time'] =
        array(
        '#type'         => 'checkbox',
        '#title'        => t('Derrived Time'),
        '#desciption'   => t('Display Derrived Time'),
        '#default_value'=> isset($defaults[$i]['display_derrived_time']) ? $defaults[$i]['display_derrived_time'] : null,
        );
        $vkey = '$derived_state_key_' . $i;
        $$vkey = ":input[name=\"osg_singout_sync_calendar[$i]['display_derrived_time']\"]";
        */
        $form['configuration']['osg_singout_sync_calendar'][$i]['derrived_time_label'] =
            array(
            '#type' => 'textfield',
            '#title' => t('Derrived Time Label'),
            '#size' => 128,
            '#desciption' => t('Display label for derrived time.'),
            '#default_value' => isset($defaults[$i]['derrived_time_label']) ? $defaults[$i]['derrived_time_label'] :
                'Derrived Time',
            // '#states'        => array('invisible' => array($$vkey => array('checked'=> false),       ),          ),
            );
        $form['configuration']['osg_singout_sync_calendar'][$i]['weight'] =
            array(
            '#type' => 'textfield',
            '#title' => t('Weight'),
            '#size' => 10,
            '#desciption' => t('Order in which to process urls.'),
            '#default_value' => isset($defaults[$i]['weight']) ? $defaults[$i]['weight'] :
                10,
            // '#states'        => array('invisible' => array($$vkey => array('checked'=> false),       ),          ),
            );
        $options = array();

        foreach ($roles as $key => $value)
        {
            $options[$key] = $key;
        }

        $fudge = array(
            'anonymous user' => 'anonymous user',
            'authenticated user' => 'authenticated user',
            'member' => 'member');
        $form['configuration']['osg_singout_sync_calendar'][$i]['roles'] = array(
            '#type' => 'checkboxes',
            '#title' => t('Role Visibility'),
            '#description' => t('What roles can see this calendar?'),           
            '#default_value' => isset($defaults[$i]['roles']) ? $defaults[$i]['roles'] :
                $fudge,
            '#options' => $options,
            );
        //----

        $button_key = ":input[name=\"osg_singout_sync_calendar[$i]['url']\"]";
    }
    //debug($button_key);
    $form['configuration']['add_calendar'] = array(
        '#type' => 'button',
        '#value' => t('Add A Calendar'),
        '#href' => '',
        '#ajax' => array(
            'callback' => 'osg_singout_sync_ajax_add_calendar_url',
            'wrapper' => 'container',
            'effect' => 'fade',

            ),
        '#states' => array('invisible' => array($button_key => array('value' =>
                        ''), ), ),

        );
    ///-------------------------------------------------------------------------------------
    $fudge = array();
    for ($i = 5; $i < 20; $i++)
    {
        $fudge[] = $i;
    }
    $form['configuration']['osg_singout_sync_zoom'] = array(
        '#type' => 'select',
        '#title' => t('Default Google map zoom level'),
        '#default_value' => variable_get('osg_singout_sync_zoom', 15),
        '#options' => drupal_map_assoc($fudge),
        );
    $form['configuration']['osg_singout_sync_default_user'] = array(
        '#type' => 'textfield',
        '#title' => t('Default <em>Sync</em> user'),
        '#autocomplete_path' => 'osg/singout/sync/user/autocomplete',
        '#default_value' => variable_get('osg_singout_sync_default_user'),
        '#description' => t('User nodes should be attributed to, when sync current user is admin.'),
        '#size' => 30,
        );
    $form['configuration']['osg_singout_sync_availability_end'] = array(
        '#type' => 'textfield',
        '#title' => t('<b>Availability </b ends x in days'),
        '#default_value' => variable_get('osg_singout_sync_availability_end', 60),
        '#description' => t('<i>Extreme</i> number of days within which to plublish nodes.'),
        '#size' => 30,
        );
    $form['configuration']['osg_singout_sync_availability_start'] = array(
        '#type' => 'textfield',
        '#title' => t('<b>Availability </b> starts x days'),
        '#default_value' => variable_get('osg_singout_sync_availability_start',
            -1),
        '#description' => t('<i>Mean</i> number of days after which to unplublish nodes.'),
        '#size' => 30,
        );
    $form['configuration']['osg_singout_sync_promote'] = array(
        '#type' => 'textfield',
        '#title' => t('<b>Promote</b> time in days'),
        '#default_value' => variable_get('osg_singout_sync_promote', 7),
        '#description' => t('<i>Promote</i> node to front page with this number of days.'),
        '#size' => 30,
        );
    $form['configuration']['osg_singout_sync_default_lead_time'] = array(
        '#type' => 'textfield',
        '#title' => t('Default Lead time'),
        '#default_value' => variable_get('osg_singout_sync_default_lead_time',
            60),
        '#description' => t('Initial offset time.'),
        '#size' => 30,
        );

    //$form['configuration'][]['#submit'][] = 'osg_singout_sync_form_submit';
    $form['configuration']['osg_singout_sync_sync_now'] = array(
        '#type' => 'submit',
        '#value' => t('Sync Now'),
        '#weight' => 50,
        '#submit' => array('osg_singout_sync_sync_now_pressed'),
        );
    $form['#submit'][] = 'osg_singout_sync_form_singout_additonal_action';
    $form['#submit'][] = 'osg_singout_sync_process_calendar_urls';
    $form['#submit'][] = '_reset_control_count';

    //$calendar_count++;
    if (isset($form_state['storage']['counter']))
    {
        $form_state['storage']['counter']++;
    }
    else
    {
        $form_state['storage']['counter'] = 2;
    }
    //debug($form);
    return system_settings_form($form);
}

function osg_singout_sync_ajax_add_calendar_url($form, $form_state)
{
    return $form['configuration']['osg_singout_sync_calendar'];
}
function osg_singout_sync_process_calendar_urls($form, &$form_state)
{
    //debug($form_state);
    $raw = $form_state['values']['osg_singout_sync_calendar'];
    $fancy = osg_sinout_sync_squish_values($raw);
    $form_state['values']['osg_singout_sync_calendar'] = $fancy;
}
function osg_sinout_sync_squish_values($ugly)
{
    $pretty = array();
    foreach ($ugly as $item)
    {
        if (is_array($item))
        {
            //debug($item);
            if (key_exists('url', $item))
            {
                if ($item['url'] > '')
                {
                    $pretty[] = $item;
                }
            }
        }
    }
    //debug($pretty);
    uasort($pretty, 'drupal_sort_weight');
    return $pretty;
}

function _reset_control_count()
{
    $_SESSION['osg_singout_sync_form']['control_count'] = 1;

}


function osg_singout_sync_get_roles()
{
    $roles = db_select('role', 'r')->fields('r', array('name', 'rid'))->execute()->
        fetchAllKeyed();
    return $roles;
}
