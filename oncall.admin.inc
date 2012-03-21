<?php
/**
 * OnCall Config Page
 * - List everyone on the team
 * - Present options for changing default settings
 */
function oncall_page_config() {
  // check that the Twilio library loads and that configuration settings have been set
  $info = libraries_load('twilio-php');
  if (!$info['loaded']) {
    drupal_set_message(t('The Twilio API for PHP library is not loading'), 'warning');
  }
  
  $twilio_config = twiliophp_config_load();
  if (!isset($twilio_config['account_sid'])) {
    drupal_set_message(t('The Twilio account settings for Account SID and Auth Token have not yet been set.'), 'warning'); 
  }
  
  // check that the On Call phone number has been set
  if (!variable_get('oncall_phone_number', "")) {
    drupal_set_message(t('You have not yet set an') . ' ' . l(t('application phone number.'), 'admin/config/services/oncall/extra'), 'warning');
  }

  $rows = array();
  $header = array(t('Person'), t('Phone Number'), t('Point Person'));
  foreach (oncall_get_team() as $member) {
    $lead = ($member['leader'] == 1) ? 'Yes' : 'No';
    $rows[] = array(l($member['name'], 'user/' . $member['uid']), $member['phone'], $lead);
  }
  
  $out  = "<h3>The On Call Team</h3>";
  $out .= theme('table', array('header' => $header, 'rows' => $rows));
  $out .= drupal_render(drupal_get_form('oncall_config_form'));
  
  return $out;
}


/**
 * #################################
 *    FORMS and FORM HANDLERS
 * #################################
 * cluster all these things togother
 */
 
/**
 * hook_form_alter()
 *  - add telephone number and availability to user edit page
 */
function oncall_form_alter(&$form, &$form_state, $form_id) {
  if ($form_id != 'user_profile_form') {
    return;
  }

  $account = $form['#user'];

  if (array_key_exists(variable_get('oncall_user_roll', 3), $account->roles)) {
    $oncall_user = oncall_user_settings($account->uid);
  
    $form['oncall'] = array(
      '#type' => 'fieldset',
      '#title' => t('On Call Settings'),
      '#weight' => 1,
    );
    
    $form['oncall']['oc_phone'] = array(
      '#type' => 'textfield', 
      '#title' => t('Phone Number'), 
      '#default_value' => isset($oncall_user->phone) ? $oncall_user->phone : '12065551212', 
      '#size' => 30, 
      '#maxlength' => 15, 
      '#description' => t('Enter the phone number you\'d like to have your OnCall SMS messages sent. Enter only numbers in this field, and include the country code. For US numbers, including the country code means starting with the number 1'),
    );
    
    $form['oncall']['oc_active'] = array(
      '#type' => 'radios',
      '#title' => t('On Call Availability'),
      '#default_value' => isset($oncall_user->active) ? $oncall_user->active : 1,
      '#options' => array(0 => t('Away'), 1 => t('Available')),
      '#description' => t('When set to "Available", sms messages from the On Call service may be sent to the telephone number given above.'),
    );

    $form['#validate'][] = 'oncall_user_form_validate';
    $form['#submit'][] = 'oncall_user_form_submit';
  } 
}
 
/**
 * Validate the phone number
 * - some phone validators found around the web
 */
function oncall_user_form_validate($form, &$form_state) {
  $phone_validator = array( 
    'us' => '/^(1([-\. ]?\d{3})[-\. ]?)?(\d{3})([-\. ]?\d{4})$/',
    'uk' => '/^(\44\s?7\d{3}|\(?07\d{3}\)?)\s?\d{3}\s?\d{3}$/',
    'uae' => '/^(\97[\s]{0,1}[\-]{0,1}[\s]{0,1}1|0)50[\s]{0,1}[\-]{0,1}[\s]{0,1}[1-9]{1}[0-9]{6}$/',
    'nz' => '/^\([0]\d{2}\))(\d{6,7}$/',
  );

  $validate = variable_get('oncall_validate_phone', 'none');
  if (isset($phone_validator[$validate])) {
    if (!preg_match($phone_validator[$validate], $form_state['values']['oc_phone'])) {
      form_set_error('oc_phone', t('That does not look like a valid telephone number'));
    }  
  }
}

/** 
 * Store the user data in our 'oncall_team' table
 */
function oncall_user_form_submit($form, &$form_state) {
  $account = $form['#user'];
  $phone = preg_replace('/\D/', '', $form_state['values']['oc_phone']);
  
  // find if this user is already in our table
  $exists = db_query("SELECT COUNT(*) as num FROM {oncall_team} WHERE uid = :uid", array(":uid" => $account->uid))->fetchField();

  if ($exists) {
    db_update('oncall_team') 
      ->fields(array(
        'phone' => $phone,
        'available' => $form_state['values']['oc_active'],
      ))
      ->condition('uid', $account->uid, '=')
      ->execute();
  }
  else {
    db_insert('oncall_team')
      ->fields(array(
        'uid' => $account->uid,
        'phone' => $phone,
        'available' => $form_state['values']['oc_active'],
      ))
      ->execute();
  }
} 

/**
 * OnCall config page settings
 */
function oncall_config_form($form, &$form_state) {
  $team = oncall_get_team();
  $team_select = array();
  $leader = 0;
  $oncall_role = user_role_load(variable_get('oncall_user_roll', 3));

  foreach ($team as $member) {
    if ($member['leader'] == 1) {
      $leader = $member['uid'];
    }
    $team_select[$member['uid']] = $member['name'];
  }
  
  // instruction for adding team members
  $ins  = "<p>To add members to your On Call team, follow these instructions:<ol>";
  $ins .= "<li>On Call members are Drupal Users. If your team member isn't already a Drupal User " . l('make them one.', 'admin/people/create') . "</li>";
  $ins .= "<li>From the " . l('user list page', 'admin/people') . ", check the box next to the names of the people to add to your team</li>";
  $ins .= "<li>Use the 'Update Options' drop down menu and under the sub-head 'Add a role to the selected users', select the option '" . $oncall_role->name . "'</li>";
  $ins .= "<li>Press the 'Update' button</li>";
  $ins .= "<li>Each team member needs to have a telephone number registered. Click the 'edit' option for the team member to add a phone number</li>";
  $ins .= "<li>Scroll to the section named 'On Call Settings', and enter the phone number there.</li>";  
  $ins .= "<li>Come back to this page - your team will be viewable in the table above</li>";
  $ins .= "</ol></p>";
  
  $form['oncall_info'] = array(
    '#type' => 'fieldset', 
    '#title' => t('Instructions for Adding Members to your On Call Team'), 
    '#collapsible' => TRUE, 
    '#collapsed' => TRUE,
  );
  $form['oncall_info']['instructions'] = array(
    '#markup' => $ins,
  );
  
  // select team leader
  $form['oncall_point'] = array(
    '#type' => 'select', 
    '#title' => t('On Call Point Person'), 
    '#default_value' => $leader, 
    '#options' => $team_select, 
    '#empty_option' => t('No Leader'), 
    '#tree' => FALSE, 
    '#description' => t('Select the person who is leading the team. A caller to this service is given the option to connect directly to the leader. If no leader is selected, the direct connect option is not available.'),
  );
  
  // sms message
  $form['oncall_sms'] = array(
    '#type' => 'textarea',
    '#title' => t('SMS Message Text'),
    '#description' => t('The message to be sent when a new help request comes in. Include the variable @url to include a link to listen to the message.'),
    '#default_value' => variable_get('oncall_sms_msg', 'A new help request has been received. Listen here @url'),
    '#rows' => 3,
  );
  
  // welcome message
  $form['welcome'] = array(
    '#type' => 'fieldset', 
    '#title' => t('Welcome Message'), 
  );
  $form['welcome']['welcome_current'] = array(
    '#markup' => theme('oncall_audio', array('src' => oncall_get_message('welcome'))),
  );
  $form['welcome']['oncall_welcome'] = array(
    '#type' => 'file',
    '#size' => 22,
    '#description' => t('The mp3 audio file that will be played every time a person calls the On Call line.'),
  );
  
  // 'no status' message
  $form['nostatus'] = array(
    '#type' => 'fieldset', 
    '#title' => t('No New Status Message'), 
  );
  $form['nostatus']['nostatus_current'] = array(
    '#markup' => theme('oncall_audio', array('src' => oncall_get_message('nostatus'))), 
  );
  $form['nostatus']['oncall_nostatus'] = array(
    '#type' => 'file',
    '#size' => 22,
    '#description' => t('The mp3 audio file that will be played when there are no recent status messages to report.'),
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  
  return $form;
}

/**
 * OnCall config_form validator
 *  - check that the file has an mp3 extension
 */
function oncall_config_form_validate($form, &$form_state) {
  $validators = array(
    'file_validate_extensions' => array('mp3'), 
  );

  // Save the 'welcome' message as a temporary file.
  $welcome = file_save_upload('oncall_welcome', $validators);
  if ($welcome === FALSE) {
    form_set_error('oncall_welcome', t('Not able to save that file. Make sure your files directory is writeable and that the file is an MP3'));
  }
  elseif ($welcome !== NULL) {
    $form_state['values']['oncall_welcome'] = $welcome;
  }

  // Save the 'no status' message as a temporary file.
  $nostatus = file_save_upload('oncall_nostatus', $validators);
  if ($nostatus === FALSE) {
    form_set_error('oncall_nostatus', t('Not able to save that file. Make sure your files directory is writeable and that the file is an MP3'));
  }
  elseif ($nostatus !== NULL) {
    $form_state['values']['oncall_nostatus'] = $nostatus;
  }
}

/**
 * OnCall config_form submit handler
 *  - override the default variables
 *   - reset the leader
 *   - point to new audio files
 */
function oncall_config_form_submit($form, &$form_state) {
  // set all team members to zero, then update the team member to leader
  db_update('oncall_team')
    ->fields(array('leader' => 0))
    ->execute();

  db_update('oncall_team')
    ->fields(array('leader' => 1))
    ->condition('uid', $form_state['values']['oncall_point'], '=')
    ->execute();
    
  variable_set('oncall_sms_msg', $form_state['values']['oncall_sms']);

  $welcome_msg = $form_state['values']['oncall_welcome'];
  oncall_save_uploaded_message($welcome_msg, 'welcome');

  $nostatus_msg = $form_state['values']['oncall_nostatus'];
  oncall_save_uploaded_message($nostatus_msg, 'nostatus');
}

/**
 * OnCall config extra settings 
 *  - set additional variables
 */
function oncall_config_extra_form($form, &$form_state) {
  $form['intro'] = array(
    '#type' => 'item', 
    '#markup' => t('A few additional settings for the On Call module'),
  );

  $form['oncall_phone_number'] = array(
    '#type' => 'textfield',
    '#title' => t('Application Phone Number'),
    '#default_value' => variable_get('oncall_phone_number', ''),
    '#description' => t('Your application phone number is your Twilio number that will be used to access this On Call module. This is needed here to send SMS messages.'),
  );

  $form['oncall_user_roll'] = array(
    '#type' => 'select', 
    '#title' => t('The User Role that tracks team members'), 
    '#default_value' => variable_get('oncall_user_roll', 2), 
    '#empty_option' => t('No Role'), 
    '#options' => user_roles(), 
    '#tree' => FALSE, 
    '#description' => t('Members of this User Role will be able to set their telephone number and be tracked as a part of the On Call team.'),
  );

  $form['oncall_validate_phone'] = array(
    '#type' => 'select', 
    '#title' => t('Phone Number Validation'), 
    '#default_value' => variable_get('oncall_validate_phone', 'none'), 
    '#empty_option' => t('No Validation'), 
    '#options' => array('us' => 'United States', 'uk' => 'United Kingdom', 'uae' => 'United Arab Emirates', 'nz' => 'New Zealand'), 
    '#tree' => FALSE, 
    '#description' => t('Should the phone number entered by team members be validated? And if so, what country phone number pattern should it be validated against?'),
  );

  // times are 6 hours, 12 hours, 18 hours, 1 day, 2 days, 1 week --- 6 hours = 6*60*60 = 21600
  $form['oncall_expire_status_msg'] = array(
    '#type' => 'select', 
    '#title' => t('Status Message Expiration'), 
    '#default_value' => variable_get('oncall_expire_status_msg', 86400), 
    '#options' => array(0 => t('Never')) + drupal_map_assoc(array(21600, 43200, 64800, 86400, 172800, 604800), 'format_interval'),
    '#tree' => FALSE,
    '#description' => t('How much time should pass before the most recent status message expires? Expired messages will not play on the phone message.'),
  );

  return system_settings_form($form);
}


/**
 * FORM HELPERS
 *
 */
 
/**
 * Retreive the user settings
 */
function oncall_user_settings($uid) {
  return db_query("SELECT * FROM {oncall_team} WHERE uid = :uid", array(":uid" => $uid))->fetchObject();
}

/**
 * Save uploaded 'canned' messages
 *  - saves the default messages to the file system
 *  - helper function for the oncall_config_form save handler
 */
function oncall_save_uploaded_message($file, $name) {
  if (!empty($file->fid)) {
    if (!$file->status) {

      // Prepare the oncall directory.
      $msg_directory =  file_default_scheme() . '://oncall';
      file_prepare_directory($msg_directory, FILE_CREATE_DIRECTORY);
      $destination = file_stream_wrapper_uri_normalize($msg_directory . '/' . $name . '-' . REQUEST_TIME . '.mp3');

      // Move the temporary file into the final location.
      if ($file = file_move($file, $destination, FILE_EXISTS_RENAME)) {
        $file->status = FILE_STATUS_PERMANENT;
        $file = file_save($file);
        variable_set('oncall_' . $name, $file->fid);
      }

    }
  }
}


