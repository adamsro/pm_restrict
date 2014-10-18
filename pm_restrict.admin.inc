<?php

/**
 * @file
 * Administration menu callbacks for pm_restrict.module.
 */

/**
 * Builds row of sending, receiving roles and actions that go with them.
 *
 * @param array $details
 *   Details of the row: default values and the unique row number (delta).
 *
 * @return array
 *   Part of a form with controls for sending, receiving and actions.
 */
function _pm_restrict_actions_form($details) {
  $form = array(
    '#tree' => TRUE,
  );
  $delta = $details['delta'];
  // FALSE by default, or if the user has checked the 'Enabled' check box for
  // this row.
  $row_disabled = (isset($details['enabled']) ? !$details['enabled'] : FALSE);

  $form['author'] = array(
    '#type' => 'textfield',
    '#autocomplete_path' => 'messages/restrict/autocomplete',
    '#default_value' => (isset($details['author']) ? $details['author'] : ''),
    '#disabled' => $row_disabled,
  );
  $form['recipient'] = array(
    '#type' => 'textfield',
    '#autocomplete_path' => 'messages/restrict/autocomplete',
    '#default_value' => (isset($details['recipient']) ? $details['recipient'] : ''),
    '#disabled' => $row_disabled,
  );

  $form['action']  = array(
    '#type' => 'radios',
    '#options' => array(
      PM_RESTRICT_ALL_RECIPIENTS => t('Send to all recipients'),
      PM_RESTRICT_MULTIPLE_SELECTIONS => t('Allow selection of recipients'),
    ),
    '#disabled' => $row_disabled,
    '#default_value' => (isset($details['action']) ? $details['action'] : PM_RESTRICT_ALL_RECIPIENTS),
  );

  $form['enabled'] = array(
    '#type' => 'checkbox',
    '#default_value' => (isset($details['enabled']) ? $details['enabled'] : TRUE),
  );
  $form['weight'] = array(
    '#type' => 'weight',
    '#delta' => 10,
    '#default_value' => (isset($details['weight']) ? $details['weight'] : TRUE),
    '#attributes' => array('class' => array('restrict-weight')),
  );
  $form['remove'] = array(
    '#type' => 'submit',
    '#submit' => array('pm_restrict_remove_submit'),
    '#value' => t('Remove_@delta', array('@delta' => $delta)),
    '#attributes' => array('class' => array('remove-action')),
    '#prefix' => '<div id="remove-rule-button">',
    '#suffix' => '<label for="edit-remove">' . t('Remove rule') . '</label></div>',
    '#ajax' => array(
      'callback' => 'pm_restrict_js',
      'wrapper' => 'block-actions',
    ),
  );
  return $form;
}

/**
 * Remove inputs from form leaving only settings to be saved.
 *
 * @param array $settings
 *   The array of settings to filter.
 *
 * @return array
 *   Array of settings, ready to be stored in the database.
 *
 * @see pm_restrict_settings_submit()
 */
function pm_restrict_settings_filter($settings) {
  // Add-in the names of any settings to be saved into the array below.
  $save_keys = array('author', 'recipient', 'action', 'enabled', 'weight');
  $matching = array();
  // Run through each of the keys we want to save, creating a new array.
  // It's not possible to simply check for unwanted values and unset() them as
  // the array is multi-dimensional.
  foreach ($save_keys as $save_key) {
    if (isset($settings[$save_key])) {
      $matching[$save_key] = $settings[$save_key];
    }
  }
  if (count($matching) > 0) {
    return $matching;
  }
  else {
    return array_map('pm_restrict_settings_filter', $settings);
  }
}

/**
 * Menu callback for AHAH handling.
 */
function pm_restrict_js($form, $form_state) {
  return drupal_render($form['block_actions']);
}

/**
 * Submit handler for 'More' button, adds a new action.
 *
 * @see pm_restrict_remove_submit()
 */
function pm_restrict_more_submit($form, &$form_state) {
  unset($form_state['submit_handlers']);
  form_execute_handlers('submit', $form, $form_state);
  // Get the submitted actions, then put them into a special area of
  // the $form_state.
  $submitted_values = $form_state['values'];
  // Add an empty action.
  $submitted_values['block_actions'][] = array();
  $form_state['pm_restrict'] = $submitted_values;
  // Rebuild the form by passing our $form_state through the
  // pm_restrict_settings() builder function.
  $form_state['rebuild'] = TRUE;
}

/**
 * Submit handler for 'Remove' button, removes an action.
 *
 * @see pm_restrict_more_submit()
 */
function pm_restrict_remove_submit($form, &$form_state) {
  unset($form_state['submit_handlers']);
  form_execute_handlers('submit', $form, $form_state);
  $submitted_values = $form_state['values'];
  // Remove the requested action.
  $delta = $form_state['clicked_button']['#parents'][1];
  unset($submitted_values['block_actions'][$delta]);
  $form_state['pm_restrict'] = $submitted_values;
  $form_state['rebuild'] = TRUE;
}

/**
 * Menu callback for blocked user settings.
 */
function pm_restrict_settings($form, &$form_state) {
  // Need to cache form for AHAH, so it can be rebuilt from cache later.
  $form = array(
    '#cache' => TRUE,
    '#attached' => array(
      'css' => array(
        drupal_get_path('module', 'pm_restrict') . '/pm_restrict.css',
      ),
    ),
  );

  // Container for just the actions, used for AHAH.
  $form['block_actions'] = array(
    '#tree' => TRUE,
    '#prefix' => '<div id="block-actions">',
    '#suffix' => '</div>',
    '#theme' => 'pm_restrict_actions',
  );

  // Should we populate the form with data from $form_state or the database?
  if (!isset($form_state['pm_restrict']['block_actions'])) {
    $block_actions = variable_get('pm_restrict_actions', array());
  }
  else {
    $block_actions = $form_state['pm_restrict']['block_actions'];
  }
  // Work through each rule, adding it as a new element in
  // $form['block_actions'] ready to be themed later.
  foreach ($block_actions as $delta => $details) {
    $details['delta'] = $delta;
    $form['block_actions'][$delta] = _pm_restrict_actions_form($details);
  }

  // The magic AHAH callback button that adds more rows.
  $form['pm_block_actions_more'] = array(
    '#type' => 'submit',
    '#value' => t('More'),
    '#weight' => 1,
    '#prefix' => '<div id="add-rule-button">',
    '#suffix' => '<label for="edit-pm-block-actions-more">' . t('Add new rule') . '</label></div>',
    '#submit' => array('pm_restrict_more_submit'),
    '#ajax' => array(
      'callback' => 'pm_restrict_js',
      'wrapper' => 'block-actions',
      'method' => 'replace',
      'effect' => 'fade',
    ),
  );

  $form['submit_form'] = array(
    '#type' => 'submit',
    '#weight' => 10,
    '#value' => t('Save configuration'),
  );

  return $form;
}

/**
 * Validate handler for admin form. Check author and recip. strings are valid.
 *
 * TODO: Does not find duplicates.
 */
function pm_restrict_settings_validate($form, &$form_state) {
  // We don't want it to submit when we're adding/removing actions.
  if ($form_state['clicked_button']['#id'] == 'edit-submit-form' && isset($form_state['values']['block_actions'])) {
    foreach ($form_state['values']['block_actions'] as $key => $action) {
      list($valid, $invalid, $duplicates, $denieds) = _privatemsg_parse_userstring($form_state['values']['block_actions'][$key]['author']);
      if (!empty($invalid)) {
        form_set_error('block_actions][' . $key . '][author', t('The following items are invalid: @invalid.', array('@invalid' => implode(", ", $invalid))));
      }
      list($valid, $invalid, $duplicates, $denieds) = _privatemsg_parse_userstring($form_state['values']['block_actions'][$key]['recipient']);
      if (!empty($invalid)) {
        form_set_error('block_actions][' . $key . '][recipient', t('The following items are invalid: @invalid.', array('@invalid' => implode(", ", $invalid))));
      }
    }
  }
}

/**
 * Submit handler for admin form.
 */
function pm_restrict_settings_submit($form, &$form_state) {
  // We don't want it to submit when we're adding/removing actions.
  if ($form_state['clicked_button']['#id'] == 'edit-submit-form') {
    // If the form's 'block_actions' aren't set, the user has deleted all the
    // rows in the table, so we save an empty array to stop errors in the form
    // builder.
    if (isset($form_state['values']['block_actions'])) {
      $weight = array();
      $block_actions = pm_restrict_settings_filter($form_state['values']['block_actions']);
      foreach ($block_actions as $key => $row) {
        $weight[$key]  = $row['weight'];
      }
      array_multisort($block_actions, SORT_DESC, SORT_STRING, $weight);
      variable_set('pm_restrict_actions', $block_actions);
    }
    else {
      variable_set('pm_restrict_actions', array());
    }
    drupal_set_message(t('The configuration options have been saved.'));
  }
}

/**
 * Return autocomplete results for usernames.
 *
 * Modified so all completions are available to anyone with privatemsg admin
 * privileges. Prevents usernames from being used and/or suggested twice.
 *
 * @see privatemsg_autocomplete()
 */
function pm_restrict_autocomplete($string) {
  $names = array();
  // 1: Parse $string and build list of valid user names.
  $fragments = explode(',', $string);
  foreach ($fragments as $name) {
    if ($name = trim($name)) {
      $names[$name] = $name;
    }
  }
  // 2: Find the next user name suggestion.
  $fragment = array_pop($names);
  $matches = array();
  if (!empty($fragment)) {
    $remaining = 10;
    $types = privatemsg_recipient_get_types();
    foreach ($types as $name => $type) {
      if (isset($type['autocomplete']) && is_callable($type['autocomplete'])) {
        $function = $type['autocomplete'];
        $return = $function($fragment, $names, $remaining, $name);
        if (is_array($return) && !empty($return)) {
          $matches = array_merge($matches, $return);
        }
        $remaining = 10 - count($matches);
        if ($remaining <= 0) {
          break;
        }
      }
    }
  }

  // Remove user 1, this user will always have all privileges.
  foreach ($matches as $key => $match) {
    if ($match->type == 'user' && $match->uid == 1) {
      unset($matches[$key]);
    }
  }

  // Format the suggestions.
  $themed_matches = array();
  foreach ($matches as $key => $match) {
    $themed_matches[$key] = privatemsg_recipient_format($match, array('plain' => TRUE));
  }

  // Check if there are any duplicates.
  if (count(array_unique($themed_matches)) != count($themed_matches)) {
    // Loop over matches, look for duplicates of each one.
    foreach ($themed_matches as $themed_match) {
      $duplicate_keys = array_keys($themed_matches, $themed_match);
      if (count($duplicate_keys) > 1) {
        // There are duplicates, make them unique.
        foreach ($duplicate_keys as $duplicate_key) {
          // Reformat them with unique argument.
          $themed_matches[$duplicate_key] = privatemsg_recipient_format($matches[$duplicate_key], array('plain' => TRUE, 'unique' => TRUE));
        }
      }
    }
  }

  // Prefix the matches and convert them to the correct form for autocomplete.
  $prefix = count($names) ? implode(", ", $names) . ", " : '';
  $suggestions = array();
  foreach ($themed_matches as $match) {
    $suggestions[$prefix . $match . ', '] = $match;
  }

  // Convert to object to prevent drupal bug, see http://drupal.org/node/175361
  drupal_json_output((object) $suggestions);
}
