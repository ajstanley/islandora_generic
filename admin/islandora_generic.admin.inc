<?php

/**
 * @file
 * islandora_generic.admin.inc
 */

/**
 *
 * @return type
 */
function islandora_generic_admin() {
  $form = array();
  $form['islandora_generic_collection_pid'] = array(
    '#type' => 'textfield',
    '#title' => t('Collection PID'),
    '#default_value' => variable_get('islandora_generic_collection_pid', ''),
    '#description' => t('PID of collection object holding replicated Dropbox structure'),
    '#weight' => -10
  );
  $form['islandora_generic_namespace'] = array(
    '#type' => 'textfield',
    '#title' => t('Namespace'),
    '#default_value' => variable_get('islandora_generic_namespace', 'islandora'),
    '#description' => t('Namespace for all objects within the replicated file structure'),
    '#weight' => -10
  );


  return system_settings_form($form);
}
