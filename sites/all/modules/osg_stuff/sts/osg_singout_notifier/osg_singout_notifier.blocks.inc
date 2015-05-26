<?php

/**
 * @file
 * Module file for osg_singout_notifier block definiton.
 */


/**
 *
 * Implements hook_menu_alter()
 *
 * Alter the data being saved to the {menu_router} table after hook_menu is invoked.
 *
 */
function osg_singout_notifier_menu_alter(&$items)
{
    $items['osg/singout/notifier/block/blasted'] = array(
        //'type' => MENU_CALLBACK,
        'title' => 'STS Singout Notifier: Registration Messages Recently Blasted',
        'page callback' => 'osg_singout_notifier_blasted_block_page',
        //'access callback' => true,

        );
    return $items;

}


/**
 * Simple page function to explain what the block example is about.
 */
function osg_singout_notifier_blasted_block_page()
{
    $page = array(
        '#type' => 'markup',
        '#markup' => t('The Singout Recent Notifications Block shows a list of Registration messages recently blasted.'),
        );
    return $page;
}
/**
 * Implements hook_block_info().
 *
 * This hook declares what blocks are provided by the module.
 */
function osg_singout_notifier_block_info()
{
    // This hook returns an array, each component of which is an array of block
    // information. The array keys are the 'delta' values used in other block
    // hooks.

    // The required block information is a block description, which is shown
    // to the site administrator in the list of possible blocks. You can also
    // provide initial settings for block weight, status, etc.


    // This sample shows how to provide default settings. In this case we'll
    // enable the block in the first sidebar and make it visible only on
    // 'node/*' pages. See the hook_block_info() documentation for these.
    $blocks['osg_singout_notifier_recent'] = array(
        // info: The name of the block.
        'info' => t('STS Singout Notifier: Registration Messages Recently Blasted'),
        'status' => TRUE,
        'region' => 'sidebar_first', // Not usually provided.
        'visibility' => BLOCK_VISIBILITY_LISTED, // Not usually provided.
        'pages' => 'node/*', // Not usually provided here.
        );


    return $blocks;
}

/**
 * Implements hook_block_configure().
 *
 * This hook declares configuration options for blocks provided by this module.
 */
function osg_singout_notifier_block_configure($delta = '')
{
    // The $delta parameter tells us which block is being configured.
    // In this example, we'll allow the administrator to customize
    // the text of the 'configurable text string' block defined in this module.

    $form = array();

    $config = array('osg_singout_notifier_recent' => array(
            'min_msg_count' => 3,
            'max_msg_count' => 0,
            ));
    $config = variable_get('osg_singout_notifier_block_config', $config);
    if ($delta == 'osg_singout_notifier_recent')
    {
        // All we need to provide is the specific configuration options for our
        // block. Drupal will take care of the standard block configuration options
        // (block title, page visibility, etc.) and the save button.
        $default = NULL;
        if (!empty($config['osg_singout_notifier_recent']['min_msg_count']))
        {
            $default = $config['osg_singout_notifier_recent']['min_msg_count'];
        }

        $form['min_msg_count'] = array(
            '#type' => 'textfield',
            '#title' => t('Minimum Message Count'),
            '#size' => 10,
            '#description' => t('This Minimum number of Recent Messages to show.'),
            '#default_value' => $default,
            );
        $default = NULL;
        if (!empty($config['osg_singout_notifier_recent']['max_msg_count']))
        {
            $default = $config['osg_singout_notifier_recent']['max_msg_count'];
        }
        $form['max_msg_count'] = array(
            '#type' => 'textfield',
            '#title' => t('Maximum Message Count'),
            '#size' => 10,
            '#description' => t('This Maximum number of Recent Messages to show.'),
            '#default_value' => $default,
            );
    }
    debug($config);
    return $form;
}

/**
 * Implements hook_block_save().
 *
 * This hook declares how the configured options for a block
 * provided by this module are saved.
 */
function osg_singout_notifier_block_save($delta = '', $edit = array())
{
    //debug($delta);
    new dbug($edit);

    // We need to save settings from the configuration form.
    // We need to check $delta to make sure we are saving the right block.
    $config = variable_get('osg_singout_notifier_block_config', array());
    //We know that this value is for the current block being edited'
    //remove know values that we don't need to save'
    $model = array('osg_singout_notifier_recent' => array(
            'min_msg_count' => 3,
            'max_msg_count' => 0,
            ));

    if (!array_key_exists($delta, $config))
    {
        $config[$delta] = array();
    }
    foreach ($model[$delta] as $key => $value)
    {

        $config[$delta][$key] = $edit[$key];
    }

    variable_set('osg_singout_notifier_block_config', $config);
    debug($config);
    return;
}


/**
 * Implements hook_block_view().
 *
 * This hook generates the contents of the blocks themselves.
 */
function osg_singout_notifier_block_view($delta = '')
{
    //The $delta parameter tells us which block is being requested.
    switch ($delta)
    {
        case 'osg_singout_notifier_recent':
            $block['subject'] = 'Title of second block (example_empty)';
            $block['content'] = _blasted_block_contents($block['subject']);
            break;
    }
    return $block;
}

/**
 * A module-defined block content function.
 */
function osg_singout_notifier_block_contents($which_block)
{
    switch ($which_block)
    {
        case 'osg_singout_notifier_recent':
            // It is possible that a block not have any content, since it is
            // probably dynamically constructed. In this case, Drupal will not display
            // the block at all. This block will not be displayed.

            return _blasted_block_contents();
    }
}
/**
 * Content of Recent Blasts Block
 *
 * @param int $timelate
 * @param int $limit
 *
 * @return
 */
function _blasted_result_set($timelate = NULL, $limit = NULL)
{
    /*
    SELECT   j.uid
    , j.nid
    , COALESCE(last_notification_time,initial_notification_time)       AS notification_time
    , email
    , notification_count
    , (UNIX_TIMESTAMP()
    - COALESCE(last_notification_time,initial_notification_time)) elap
    , u.first_name
    , u.last_name
    , n.title
    , i.start_time_unix
    , s.label
    FROM     osg_singout_notifier_journal j
    INNER JOIN osg_users u
    ON j.uid = u.uid
    INNER JOIN node n
    ON n.nid = j.nid
    INNER JOIN osg_ical_imported i
    ON i.nid = n.nid
    LEFT JOIN osg_registration_ext r
    ON r.nid = j.nid
    AND r.uid = j.uid
    LEFT JOIN registration_state s
    ON r.state = s.name
    WHERE    (UNIX_TIMESTAMP()
    - COALESCE(initial_notification_time,last_notification_time)) < 86400000
    ORDER BY uid
    , nid
    */

    $query = db_select('osg_singout_notifier_journal', 'j');
    $query->join('osg_users', 'u', 'j.uid = u.uid');
    $query->join('node', 'n', 'n.nid = j.nid');
    $query->join('osg_ical_imported', 'i', 'i.nid = n.nid');
    $query->leftJoin('osg_registration_ext', 'r',
        'r.nid = j.nid AND r.uid = j.uid');
    $query->leftJoin('registration_state', 's', 'r.state = s.name');
    $snippet = 'UNIX_TIMESTAMP()-COALESCE(last_notification_time,initial_notification_time)';
    $query->addExpression('COALESCE(last_notification_time,initial_notification_time)',
        'notification_time');
    $query->fields('j', array(
        'nid',
        'uid',
        'email',
        'notification_count'));
    $query->fields('u', array('first_name', 'last_name'));
    $query->fields('n', array('title'));
    $query->fields('i', array('start_time_unix'));
    $query->fields('s', array('label'));

    if (!empty($timelate))
    {
        $query->where($snippet . '<:expiry', array(':expiry' => $timelate));

    }

    if (!empty($limit))
    {
        $query->range(0, $limit);
    }

    $result = $query->execute();

    if ($result->rowCount())
    {
        return $result;
    } else
    {
        return NULL;
    }


}
function _blasted_block_contents(&$subject)
{

    $config = variable_get('osg_singout_notifier_block_config');
    if (!empty($config['osg_singout_notifier_recent']))
    {
        foreach ($config['osg_singout_notifier_recent'] as $key => $value)
        {
            $$key = $value;
        }

    }

    if (!isset($min_msg_count))
    {
        $min_msg_count = NULL;
    }
    if (!isset($max_msg_count))
    {
        $max_msg_count = NULL;
    }
    $timestack = osg_api_chrono_array('hours|3|48|6;day|3|14;week|3|26');
    foreach ($timestack as $seconds => $label)
    {
        $result = _blasted_result_set($seconds, $max_msg_count);
        if ($result)
        {
            if ($result->rowCount() >= $min_msg_count)
            {
                break;
            }
        }

    }
    $subject="Messages sent in the last $label"; 

    $heading = array();
    $columns = array();

    $header = array(
        'Notification Time',
        'Addressee',
        'Event',
        'Notifiation Count');

    $rows[] = $columns;
    while ($record = $result->fetchAssoc())
    {
        /* Record Structure
        array(
        'nid' => '4094',
        'uid' => '635',
        'email' => 'huegel@verizon.net',
        'notification_count' => '10',
        'first_name' => 'Ann',
        'last_name' => 'Huegel',
        'title' => 'BRIGHTON GARDENS OF COLUMBIA',
        'start_time_unix' => '1424631600',
        'label' => NULL,
        'notification_time' => '1424486826',
        )

        */ //$columns[] = date('D M jS',$record['date']);
        //$columns[] = osg_sap_node_link($record['nid'],$record['title']);

        $columns[] = date('M j g:i a', $record['notification_time']);
        $addressee = trim(trim($record['first_name']) . ' ' . trim($record['last_name']));
        $addressee = $addressee . ' [' . $record['email'] . ']';
        $columns[] = $addressee;
        $columns[] = $record['title'] . ' ' . date('D M jS g:i a', $record['start_time_unix']);
        $columns[] = $record['notification_count'];


        $rows[] = $columns;
        $columns = array();

    }

    $data = array(
        'header' => $header,
        'rows' => $rows,
        );

    //$output = theme('table',$data);
    return  theme('table', $data);

}
