<?php
/**
* @file
* Example of how to use Drupal's mail API.
*/

/**
* @defgroup osg_singout_notifier Example: Email
* @ingroup examples
* @{
* Example of how to use Drupal's mail API.
*
* This example module provides two different examples of the Drupal email API:
*  - Defines a simple contact form and shows how to use drupal_mail()
*    to send an e-mail (defined in hook_mail()) when the form is submitted.
*  - Shows how modules can alter emails defined by other Drupal modules or
*    Core using hook_mail_alter by attaching a custom signature before
*    they are sent.
*/

function osg_singout_notifier_mail($key, & $message, $params) {
    global $user;

    // Each message is associated with a language, which may or may not be the
    // current user's selected language, depending on the type of e - mail being
    // sent. This $options array is used later in the t() calls for subject
    // and body to ensure the proper translation takes effect.
    $options = array(
        'langcode'=> $message['language']->language,
    );
    debug($key,'$key');
    debug($params,'$params');
    switch ($key) {
        // Send a simple message from the contact form.
        case 'contact_message':
        $message['subject'] = t('E-mail sent from @site-name', array('@site-name'=> variable_get('site_name', 'Drupal')), $options);
        // Note that the message body is an array, not a string.
        $message['body'][] = t('@name sent you the following message:', array('@name'=> $user->name), $options);
        // Because this is just user - entered text, we do not need to translate it.

        // Since user - entered text may have unintentional HTML entities in it like
        // ' < ' or ' > ', we need to make sure these entities are properly escaped,
        // as the body will later be transformed from HTML to text, meaning
        // that a normal use of ' < ' will result in truncation of the message.
        $message['body'][] = check_plain($params['message']);
        break;
        case'registration_needed':
        $message['body'][] = $params['body'];
        $message['subject'] = check_plain($params['subject']);
        //$message['headers']['Content - Type'] = $params['headers']['Content - Type'] ;
        break;

    }
    debug($message,"MESSAGE");

}


/*
///// Supporting functions ////

/**
* Implement hook_menu().
*
* Set up a page with an e-mail contact form on it.
*/
function osg_singout_notifier_menu() {
    $items['osg/singout/notifier/contact/form'] = array(
        'title'           => 'STS Singout Notifier: contact form',
        'page callback'   => 'drupal_get_form',
        'page arguments'                 => array('osg_singout_notifier_form'),
        'access arguments' => array('access content'),
    );
	
    $items['osg/singout/notifier/unregistered/form'] = array(
        'title'           => 'STS Singout Notifier: Blast Unregistered',
        'page callback'   => 'drupal_get_form',
        'page arguments'                 => array('osg_singout_notifier_blast_unregistered_form'),
        'access arguments' => array('access content'),
    );
	$items['osg/singout/notifier/blast/unregistered'] = array(
        'type'           => MENU_CALLBACK,
        'title'          =>'STS Singout Notifier: Registration Needed',
        'page callback'  => 'osg_singout_notifier_registration_needed',
        'access callback'=> TRUE,

    );

    return $items;
}

/**
* The contact form.
*/
function osg_singout_notifier_form() {
    $form['intro'] = array(
        '#markup'=> t('Use this form to send a message to an e-mail address. No spamming!'),
    );
    $form['email'] = array(
        '#type'    => 'textfield',
        '#title'   => t('E-mail address'),
        '#required'=> TRUE,
    );
    $form['message'] = array(
        '#type'    => 'textarea',
        '#title'   => t('Message'),
        '#required'=> TRUE,
    );
    $form['submit'] = array(
        '#type' => 'submit',
        '#value'=> t('Submit'),
    );

    return $form;
}

/**
* Form validation logic for the contact form.
*/
function osg_singout_notifier_form_validate($form, & $form_state) {
    if (!valid_email_address($form_state['values']['email'])) {
        form_set_error('email', t('That e-mail address is not valid.'));
    }
}

/**
* Form submission logic for the contact form.
*/
function osg_singout_notifier_form_submit($form, & $form_state) {
    osg_singout_notifier_mail_send($form_state['values']);
}
/**
* @} End of "defgroup osg_singout_notifier".
*/
function osg_singout_notifier_blast_unregistered_form() {


    $form['markup'] = array(

        '#markup'=> t('Define parameters for non-respondant email blast'),
    );
    $form['subject'] = array(
        '#type'    => 'textfield',
        '#title'   => t('Subject'),
        '#required'=> TRUE,
        '#size' => 128,
        '#desciption' => t('Subject of non-respondant email blast unregistered'),
        '#default_value' => variable_get('osg_singout_notifier_blast_unregistered_subject', 'Upcoming STS Performances'),
    );
    $default = 'The following is a list of upcoming ShowTime Performances for which you have not responded. '
    .'Please log onto  [site:name] and indicate your attendance plan.';
    $form['message_before'] = array(
        '#type'    => 'textarea',
        '#title'   => t('Message (before listing)'),
        '#description'=>t('Text of email (before list of unregistered performances).'),
        '#default'=>variable_get('osg_singout_notifier_blast_unregistered_message_before',$default),
        '#required'=> TRUE,
    );
    $form['message_after'] = array(
        '#type'    => 'textarea',
        '#title'   => t('Addtional Message'),
        '#description'=>t('Text of email (after the list of unregistered performances).'),
        '#default'=>variable_get('osg_singout_notifier_blast_unregistered_message_after'),
    );
    return system_settings_form($form);
}

/**
* Email Blast Logic
*
* @return
*/
function osg_singout_notifier_registration_needed() {
    $sql         = "SELECT u.uid ";
    $sql         = $sql .", u.name as user_name";
    $sql         = $sql .", u.mail ";
    $sql         = $sql .", a.start_time_unix ";
    $sql         = $sql .", a.title AS venue ";
    $sql         = $sql .", c.nid ";
    $sql         = $sql .", c.title ";
    $sql         = $sql .", f.field_firstname_value AS first_name ";
    $sql         = $sql .", l.field_lastname_value AS last_name ";
    $sql         = $sql ."FROM ( ";
    $sql         = $sql ."SELECT * ";
    $sql         = $sql ."FROM osg_ical_imported ";
    $sql         = $sql ."WHERE available =1 ";
    $sql         = $sql .")a ";
    $sql         = $sql ."INNER JOIN ( ";
    $sql         = $sql ."SELECT uid ";
    $sql         = $sql ."FROM role_permission p ";
    $sql         = $sql ."INNER JOIN users_roles r ON r.rid = p.rid ";
    $sql         = $sql ."WHERE permission = 'access singout agglomerate' AND uid NOT IN (0, 1) ";
    $sql         = $sql .")b ";
    $sql         = $sql ."INNER JOIN users u ON b.uid = u.uid ";
    $sql         = $sql ."INNER JOIN ( ";
    $sql         = $sql ."SELECT n . *, u.uid AS user_uid ";
    $sql         = $sql ."FROM node n CROSS ";
    $sql         = $sql ."JOIN users u ";
    $sql         = $sql ."LEFT JOIN registration r ON r.entity_id = n.nid AND u.uid = r.user_uid ";
    $sql         = $sql ."WHERE r.registration_id IS NULL ";
    $sql         = $sql .")c ON a.nid = c.nid AND u.uid = user_uid AND u.`status` =1 ";
    $sql         = $sql ."INNER JOIN field_data_field_firstname f ON u.uid = f.entity_id ";
    $sql         = $sql ."INNER JOIN field_data_field_lastname l ON u.uid = l.entity_id ";
    $sql         = $sql ."ORDER BY uid, start_time_unix ";

    $result      = db_query($sql);
    $hold_record = array();

    $heading = array('Event','Date','Time');

    $rows = array();
    $column = array();
    $lines = array();
    $lines[0] = "";

    if ($result) {
        while ($record = $result->fetchAssoc()) {
            // Do something with:
            //    $record['name']
            //    $record['quantity']
            if (isset($hold_record['uid']) ) {

                if ($hold_record['uid'] <> $record['uid']) {
                    _osg_singout_notifier_prep_message($hold_record,$lines);
                    $lines = array();
                    $lines[0] = "";


                }
            }
            $lines[] = $record['title']."\t".date('l, F jS Y',$record['start_time_unix'])
            ."\t". date('h:i A',$record['start_time_unix']);

            $hold_record = $record;
        }
        if (count($rows)) {
            _osg_singout_notifier_prep_message($record,$lines);
        }

    }
    drupal_goto("<front>");
}
/*
$message: An array containing the message data. Keys in this array include:

'id': The drupal_mail() id of the message. Look at module source code or drupal_mail() for possible id values.
'to': The address or addresses the message will be sent to. The formatting of this string will be validated with the
PHP e-mail validation filter.
'from': The address the message will be marked as being from, which is either a custom address or the site-wide
default email address.
'subject': Subject of the email to be sent. This must not contain any newline characters, or the email may not be sent properly.
'body': An array of strings containing the message text. The message body is created by concatenating the individual
array strings into a single text string using "\n\n" as a separator.
'headers': Associative array containing mail headers, such as From, Sender, MIME-Version, Content-Type, etc.
'params': An array of optional parameters supplied by the caller of drupal_mail()
that is used to build the message before hook_mail_alter() is invoked.
'language': The language object used to build the message before hook_mail_alter() is invoked.
'send': Set to FALSE to abort sending this email message.
*/
function _osg_singout_notifier_prep_message($info,$data) {
    global $base_url;
    $message = array();
    $separator = md5(time());
    // carriage return type (we use a PHP end of line constant)
    $eol       = PHP_EOL;

    //$params['to'] = $record['email'];
    $recipient = $info['first_name'].' '.$info['last_name'].' <csharpe@unirisc.com'.'>';
    $sender    = variable_get('site_mail', 'admin@example.com');
    $message['subject'] = 'These Performances are available for registration.';
    $fudge = count($data) > 2?'s':'';
    $fudge = "Please visit <a href=\"$base_url\">".variable_get('site_name','[Some Cool Site]')."</a>"
    ." and indicate your attendance plan for the following event$fudge:";
    $data[0] = $fudge;
    $body = implode("<br>",$data);



    $message['body'] = $body;

    debug($message,'$message');
    //drupal_mail($module, $key, $to, $language, $params = array(), $from = NULL, $send = TRUE)
    drupal_mail('osg_singout_notifier'
        , 'registration_needed'
        , $recipient
        , language_default()
        , $message
        , $sender
    );

}

