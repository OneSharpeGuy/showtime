<?php
/**
* Provide an alternate system setting form
* @param array $form
*
* @return
*/

function osg_api_system_settings_form($form)
{
  $form['actions']['#type'] = 'actions';
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value'=> t('Save configuration'),
  );

  if (!empty($_POST) && form_get_errors())
  {
    drupal_set_message(t('The settings have not been saved because of the errors.'), 'error');
  }
  $form['#submit'][] = 'osg_api_system_settings_form_submit';
  // By default, render the form using theme_system_settings_form().
  if (!isset($form['#theme']))
  {
    $form['#theme'] = 'osg_api_system_settings_form';
  }
  return $form;
}
/**
*
* @param undefined $form
* @param undefined $form_state
*
* @return
*/
function osg_api_system_settings_form_submit($form, & $form_state)
{
  // Exclude unnecessary elements.
  form_state_values_clean($form_state);
  debug($form_state['values']);
  return;
  foreach ($form_state['values'] as $key => $value)
  {
    if (is_array($value) && isset($form_state['values']['array_filter']))
    {
      $value = array_keys(array_filter($value));
    }
    variable_set($key, $value);
  }

  drupal_set_message(t('The configuration options have been saved.'));
}

?>