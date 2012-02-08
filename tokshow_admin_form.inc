<?php

function tokshow_config_form($form, $form_state) {
  $form['description'] = array(
    '#type' => 'item',
    '#title' => t('TokShow Settings'),
  );

  $form['tokshow_artist_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Arist Name'),
    '#default_value' => variable_get('tokshow_artist_name'),
    '#required' => TRUE
  );

  $form['tokshow_api_key'] = array(
    '#type' => 'textfield',
    '#title' => t('API Key'),
    '#default_value' => variable_get('tokshow_api_key'),
    '#required' => TRUE
  );

  $form['tokshow_api_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('API Secret'),
    '#default_value' => variable_get('tokshow_api_secret'),
    '#required' => TRUE
  );

  $form['tokshow_admin_email'] = array(
    '#type' => 'textfield',
    '#title' => t('Admin Email'),
    '#default_value' => variable_get('tokshow_admin_email'),
    '#required' => TRUE
  );

  $form['tokshow_admin_password'] = array(
    '#type' => 'textfield',
    '#title' => t('Admin Password'),
    '#default_value' => variable_get('tokshow_admin_password'),
    '#required' => TRUE
  );

  $form['tokshow_signin_image_fid'] = array(
    '#title' => t('Sign-in Image'),
    '#type' => 'managed_file',
    '#description' => t('This image will be shown for users that are not logged in.'), 
    '#default_value' => variable_get('tokshow_signin_image_fid', ''),
    '#upload_location' => 'public://tokshow/',
  );

  $form['tokshow_before_image_fid'] = array(
    '#title' => t('Before Show Image'),
    '#type' => 'managed_file',
    '#description' => t('Background image to show before the event.'), 
    '#default_value' => variable_get('tokshow_before_image_fid', ''),
    '#upload_location' => 'public://tokshow/',
  );

  $form['tokshow_during_image_fid'] = array(
    '#title' => t('During Show Image'),
    '#type' => 'managed_file',
    '#description' => t('Background image to show during the event.'), 
    '#default_value' => variable_get('tokshow_during_image_fid', ''),
    '#upload_location' => 'public://tokshow/',
  );

  $form['tokshow_after_image_fid'] = array(
    '#title' => t('After Show Image'),
    '#type' => 'managed_file',
    '#description' => t('Background image to show after the event.'), 
    '#default_value' => variable_get('tokshow_after_image_fid', ''),
    '#upload_location' => 'public://tokshow/',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
  );

  return $form;
}


function tokshow_config_form_validate($form, &$form_state) {
  $email = $form_state['values']['tokshow_admin_email'];
  if (!valid_email_address($email)) {
    form_set_error('tokshow_admin_email', t('Email invalid'));
  }
}

function tokshow_config_form_submit($form, &$form_state) {
  // Update the config vars
  $request_new_tokshow = false;
  if (_tokshow_update_config($form_state, "tokshow_artist_name")) $request_new_tokshow = true;
  if (_tokshow_update_config($form_state, "tokshow_api_key")) $request_new_tokshow = true;
  if (_tokshow_update_config($form_state, "tokshow_api_secret")) $request_new_tokshow = true;
  if (_tokshow_update_config($form_state, "tokshow_admin_email")) $request_new_tokshow = true;
  if (_tokshow_update_config($form_state, "tokshow_admin_password")) $request_new_tokshow = true;

  // Get the TokShow
  if ($request_new_tokshow) {
    $url = "https://api.opentok.com/hl/tokshow/create";
    $data = array(
      'artist_name' => variable_get('tokshow_artist_name'),
      'email' => variable_get('tokshow_admin_email'),
      'password' => variable_get('tokshow_admin_password'),
    );
    $data = http_build_query($data, '', '&');
    $options = array(
      'method' => 'POST',
      'data' => $data,
      'timeout' => 15,
      'headers' => array(
        'Content-Type' => 'application/x-www-form-urlencoded',
        'x-tb-partner-auth' => variable_get('tokshow_api_key') . ':' . variable_get('tokshow_api_secret'),
      ),
    );

    $result = drupal_http_request($url, $options);
    if ($result->status_message == "OK") {
      $response = drupal_json_decode($result->data);
      variable_set('tokshow_embed_id', $response['embed_id']);
      drupal_set_message('TokSok updated'); 
    } else {
      drupal_set_message('Error getting TokShow. Ensure that your API key and secret are correct. Email support@tokbox.com with issues.', 'error');
    }
  }

  // Manage the uploaded files
  _tokshow_handle_file($form_state, "tokshow_signin_image_fid");
  _tokshow_handle_file($form_state, "tokshow_before_image_fid");
  _tokshow_handle_file($form_state, "tokshow_during_image_fid");
  _tokshow_handle_file($form_state, "tokshow_after_image_fid");

  // Make stylesheet file
  $style = "";
  if ($fid = variable_get("tokshow_before_image_fid", FALSE)) {
    $file = file_load($fid);
    $filename = file_create_url( $file->uri );
    $style = $style . "body.large .stage { background: url('" . $filename . "'); }\n";
  }

  if ($fid = variable_get("tokshow_during_image_fid", FALSE)) {
    $file = file_load($fid);
    $filename = file_create_url( $file->uri );
    $style = $style . "body.large #event { background: url('" . $filename . "'); }\n"; 
  }

  if ($fid = variable_get("tokshow_after_image_fid", FALSE)) {
    $file = file_load($fid);
    $filename = file_create_url( $file->uri );
    $style = $style . "body.large .stage.end_show { background: url('" . $filename . "'); }\n";  
  }

  $f = file_save_data($style, "public://tokshow/styles.css", FILE_EXISTS_REPLACE);
  variable_set("tokshow_css_fid", $f->fid);
}

function _tokshow_update_config($form_state, $selector) {
  $var = variable_get($selector, FALSE);
  $changed = false;
  if ($var != $form_state['values'][$selector]) {
    variable_set($selector, $form_state['values'][$selector]);
    $changed = true;
  }

  return $changed;
}

function _tokshow_handle_file($form_state, $selector) {
  if ($form_state['values'][$selector] != 0) {

    $file = file_load($form_state['values'][$selector]);
    $file->status = FILE_STATUS_PERMANENT;
    file_save($file);

    file_usage_add($file, 'tokshow', 'sample_image', 1);
    variable_set($selector, $file->fid);

  } elseif ($form_state['values'][$selector] == 0) {

    $fid = variable_get($selector, FALSE);
    $file = $fid ? file_load($fid) : FALSE;

    if ($file) {
      file_usage_delete($file, 'tokshow', 'sample_image', 1);
      file_delete($file);

      variable_set($selector, FALSE);
      drupal_set_message(t('The image @image_name was removed.', array('@image_name' => $file->filename)));
    }
  }
}
