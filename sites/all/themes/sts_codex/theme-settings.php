<?php
/**
* Implements hook_form_system_theme_settings_alter().
*
* @param $form
*   Nested array of form elements that comprise the form.
* @param $form_state
*   A keyed array containing the current state of the form.
*/

function sts_codex_form_system_theme_settings_alter( & $form, & $form_state)
{

  // Include a hidden form field with the current release information
  $form['at-release'] = array(
    '#type'         => 'hidden',
    '#default_value'=> '7.x-3.x',
  );

  // Tell the submit function its safe to run the color inc generator
  // if running on AT Core 7.x - 3.x
  $form['at-color'] = array(
    '#type'         => 'hidden',
    '#default_value'=> TRUE,
  );

  // EXTENSIONS
  $enable_extensions = isset($form_state['values']['enable_extensions']);
  if (($enable_extensions && $form_state['values']['enable_extensions'] == 1) || (!$enable_extensions && $form['at-settings']['extend']['enable_extensions']['#default_value'] == 1)) {

    // Remove option to use full width wrappers
    $form['at']['modify-output']['design']['page_full_width_wrappers'] = array(
      '#access'       => FALSE,
      '#default_value'=> 0,
    );

    $form['at']['header'] = array(
      '#type'       => 'fieldset',
      '#title'      => t('Header layout'),
      '#description'=> t('<h3>Header layout</h3><p>Change the position of the logo, site name and slogan. Note that his will automatically alter the header region layout also. If the branding elements are centered the header region will center below them, otherwise the header region will float in the opposite direction.</p>'),
    );
    $form['at']['header']['header_layout'] = array(
      '#type'         => 'radios',
      '#title'        => t('Branding position'),
      '#default_value'=> theme_get_setting('header_layout'),
      '#options'             => array(
        'hl-l'=> t('Left'),
        'hl-r'=> t('Right'),
        'hl-c'=> t('Centered'),
      ),
    );
    $form['at']['menu_styles'] = array(
      '#type'       => 'fieldset',
      '#title'      => t('Menu Bullets'),
      '#description'=> t('<h3>Menu Bullets</h3><p>This setting allows you to customize the bullet images used on menus items. Bullet images only show on normal vertical block menus.</p>'),
    );
    $form['at']['menu_styles']['menu_bullets'] = array(
      '#type'         => 'select',
      '#title'        => t('Menu Bullets'),
      '#default_value'=> theme_get_setting('menu_bullets'),
      '#options'             => array(
        'mb-n' => t('None'),
        'mb-dd'=> t('Drupal default'),
        'mb-ah'=> t('Arrow head'),
        'mb-ad'=> t('Double arrow head'),
        'mb-ca'=> t('Circle arrow'),
        'mb-fa'=> t('Fat arrow'),
        'mb-sa'=> t('Skinny arrow'),
      ),
    );
    $form['at']['pagestyles'] = array(
      '#type'       => 'fieldset',
      '#title'      => t('Textures'),
      '#description'=> t('<h3>Textures</h3><p>Textures are small, semi-transparent images that tile to fill the entire background.</p>'),
    );
    $form['at']['pagestyles']['textures'] = array(
      '#type'       => 'fieldset',
      '#title'      => t('Textures'),
      '#description'=> t('<h3>Body Textures</h3><p>This setting adds a texture over the Featured and Tertiary region panels - the darker the background the more these stand out, on light backgrounds the effect is subtle.</p>'),
    );
    $form['at']['pagestyles']['textures']['body_background'] = array(
      '#type'         => 'select',
      '#title'        => t('Select texture'),
      '#default_value'=> theme_get_setting('body_background'),
      '#options'             => array(
        'bb-n' => t('None'),
        'bb-h' => t('Hatch'),
        'bb-vl'=> t('Vertical lines'),
        'bb-hl'=> t('Horizontal lines'),
        'bb-g' => t('Grid'),
      ),
    );

  }
  if (!empty( $form['at']['pagestyles']['textures']['body_background']['#options'])) {
    $form['at']['pagestyles']['textures']['body_background']['#options']['bb-sts'] = t('STS Logo');
  }

}
