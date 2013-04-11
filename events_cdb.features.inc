<?php
/**
 * @file
 * events_cdb.features.inc
 */

/**
 * Implements hook_node_info().
 */
function events_cdb_node_info() {
  $items = array(
    'event_cdb' => array(
      'name' => t('Event CDB'),
      'base' => 'node_content',
      'description' => '',
      'has_title' => '1',
      'title_label' => t('Title'),
      'help' => '',
    ),
  );
  return $items;
}
