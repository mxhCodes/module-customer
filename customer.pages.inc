<?php

/**
 * @file
 * All page callbacks for the customer module.
 */

/**
 * Customer default view.
 */
function customer_view($customer, $view_mode = 'full') {
  return entity_view('customer', array($customer->customer_id => $customer), $view_mode);
}

/**
 * Customer creation page.
 */
function customer_create_page() {
  $customer = entity_create('customer', array());
  $output = entity_ui_get_form('customer', $customer, 'add');
  drupal_set_title(t('Add new customer'));
  return $output;
}
 
/**
 * Customer default view page.
 */
function customer_view_page($customer) {
  $output = array('customer_headline' => array('#markup' => '<h2>' . $customer->name . '</h2>'));
  $output += customer_view($customer);
  
  drupal_set_title(t('Customer account for @name', array('@name' => $customer->name)));
  return $output;
}

/**
 * Customer administration view page.
 */
function customer_admin_view_page($customer) {
  $output = customer_view($customer, 'administration');
  $admin_info = '<h3>' . t('Created by') . '</h3>'
    . theme('username', array('account' => user_load($customer->uid_created)))
    . ' (' . format_date($customer->created) . ')';
  if ($customer->uid_changed != 0) {
    $admin_info .= '<h3>' . t('Changed by') . '</h3>'
      . theme('username', array('account' => user_load($customer->uid_changed)))
      . ' (' . format_date($customer->changed) . ')';
  }
  $output['customer']['admin_info'] = array(
    '#type' => 'fieldset',
    '#title' => t('Administration details'),
    '#children' => $admin_info,
    '#collapsible' => TRUE,  
    '#collapsed'  => TRUE, 
    '#attributes' => array('class' => array('collapsible', 'collapsed', 'fieldset-admin-info')),
  );
  drupal_add_js('misc/form.js');
  drupal_add_js('misc/collapse.js');

  drupal_set_title(t('Customer account for @name', array('@name' => $customer->name)));
  return $output;
}

/**
 * Form constructor for the admin customer edit form.
 */
function customer_admin_form($form, &$form_state, $customer) {
  return customer_form($form, $form_state, $customer);
}

/**
 * Form constructor for the customer add/edit form.
 *
 * @see customer_form_validate()
 * @see customer_form_submit()
 * @see customer_form_delete_submit()
 * @ingroup forms
 */
function customer_form($form, &$form_state, $customer) {
  $form_state['customer'] = $customer;
  
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name (Company or person)'),
    '#default_value' => !empty($customer->name) ? $customer->name : '',
    '#weight' => -100,
    '#required' => TRUE,
  );
  $form['owner'] = array(
    '#type' => 'textfield',
    '#title' => t('Owner'),
    '#description' => t('Type in an existing username. The owner is usually the user account of the customer. The owner can view and update his or her customer account.'),
    '#default_value' => !empty($customer->uid_owner) ? user_load($customer->uid_owner)->name : '',
    '#weight' => -90,
    '#required' => FALSE,
    '#access' => user_access('administer customer entities') ? TRUE : FALSE,
  );
  
  field_attach_form('customer', $customer, $form, $form_state, entity_language('customer', $customer));
  
  $form['actions']['#type'] = 'actions';
  $form['actions']['#weight'] = 100;
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 10,
    '#submit' => array('customer_form_submit'),
  );
  if (isset($customer->is_new) && $customer->is_new == TRUE) {
    $form['actions']['submit_add'] = array(
      '#type' => 'submit',
      '#value' => t('Save and add another one'),
      '#weight' => 20,
      '#submit' => array('customer_form_submit'),
    );
  }
  else {
    $form['actions']['submit_delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#weight' => 20,
      '#submit' => array('customer_form_delete_confirm'),
      '#access' => customer_access('delete', $customer) ? TRUE : FALSE,
    );
  }
  
  $form['#validate'][] = 'customer_form_validate';
  
  drupal_set_title(t('Edit customer account @customer', array('@customer' => $customer->name)));
  
  return $form;
}

/**
 * Form submission handler for customer_form().
 */
function customer_form_submit($form, &$form_state) {
  global $user;
  $customer = $form_state['customer'];

  entity_form_submit_build_entity('customer', $customer, $form, $form_state);

  if (isset($customer->is_new) && $customer->is_new == TRUE) {
    $customer->uid_created = $user->uid;
    $customer->created = REQUEST_TIME;
  }
  $customer->uid_changed = $user->uid;
  $customer->changed = REQUEST_TIME;
  
  if (!empty($form_state['values']['owner'])) {
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'user')
      ->propertyCondition('name', $form_state['values']['owner'])
      ->range(0, 1)
      ->addMetaData('account', user_load(1));
    $result = $query->execute();
    if (!empty($result)) {
      $item = reset($result['user']);
      $customer->uid_owner = $item->uid;
    }
  }
  customer_save($customer);
  drupal_set_message(t('Customer information for %customer has been saved.', array('%customer' => $customer->name)), 'status');
  
  if ($form_state['values']['op'] == t('Save and add another one') && user_access('administer customer entities')) {
    $redirect = current_path();
  }
  elseif (user_access('view customer entities') && arg(0) == 'admin') {
    $redirect = 'admin/customers';
  }
  elseif (customer_access('view', $customer)) {
    $uri = $customer->uri();
    $redirect = reset($uri);
  }
  else {
    $redirect = '<front>';
  }
  $form_state['redirect'] = $redirect;
}

/**
 * Form validation handler for customer_form().
 */
function customer_form_validate($form, &$form_state) {
  $customer = $form_state['customer'];
  if (isset($customer->is_new) && $customer->is_new == TRUE) {
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'customer')
      ->propertyCondition('name', $form_state['values']['name'])
      ->range(0, 1)
      ->addMetaData('account', user_load(1));
    $result = $query->execute();
    if (!empty($result)) {
      form_set_error('name', t('This customer already exists.'));
    }
  }
  if (!empty($form_state['values']['owner'])) {
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'user')
      ->propertyCondition('name', $form_state['values']['owner'])
      ->range(0, 1)
      ->addMetaData('account', user_load(1));
    $result = $query->execute();
    if (empty($result)) {
      form_set_error('owner', t('The entered username does not exist.'));
    }
  }
  entity_form_field_validate('customer', $form, $form_state);
}

/**
 * Form delete submission handler for customer_form().
 */
function customer_form_delete_confirm($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $customer = $form_state['customer'];
  if (arg(0) == 'admin') {
    $form_state['redirect'] = array('admin/customer/' . $customer->customer_id . '/delete', array('query' => $destination));
  }
  else {
    $uri = $customer->uri();
    $path = reset($uri);
    $form_state['redirect'] = array($path . '/delete', array('query' => $destination));
  }
}

/**
 * Asks the admin for customer deletion.
 */
function customer_admin_delete_confirm($form, &$form_state, $customer) {
  return customer_delete_confirm($form, $form_state, $customer);
}

/**
 * Asks for customer deletion.
 */
function customer_delete_confirm($form, &$form_state, $customer) {
  // Always provide entity id in the same form key as in the entity edit form.
  $form['customer_id'] = array('#type' => 'value', '#value' => $customer->customer_id);
  $uri = $customer->uri();
  $path = reset($uri);
  return confirm_form($form,
    t('You want to delete %name', array('%name' => $customer->name)),
    $path,
    t('Are you sure you want to delete the customer account %name?', array('%name' => $customer->name)) . ' ' . t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Executes customer deletion on admin pages.
 */
function customer_admin_delete_confirm_submit($form, &$form_state) {
  customer_delete_confirm_submit($form, $form_state);
  $form_state['redirect'] = user_access('view customer entities') ? 'admin/customers' : '<front>';
}

/**
 * Executes customer deletion.
 */
function customer_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $customer = customer_load($form_state['values']['customer_id']);
    customer_delete($customer->customer_id);
    watchdog('customer', 'Deleted the customer account %name.', array('%name' => $customer->name));
    drupal_set_message(t('The customer account %name has been deleted.', array('%name' => $customer->name)));
  }

  $form_state['redirect'] = '<front>';
}

/**
 * Customer entity settings page.
 * @todo Remove settings page when no settings are needed.
 */
function customer_settings() {
  return t('There are no general settings for customers. You can manage fields and display settings here.');
}

/**
 * Administration overview of customers.
 */
function customer_admin_overview($form, &$form_state) {

  $param = drupal_get_query_parameters();
  $user_admin_access = user_access('administer customer entities');
  
  $form['filters'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,  
    '#collapsed'  => FALSE, 
    '#title' => t('Show only items where'),
  );
  
  $form['filters']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name (Company or person)') . ' ' . t('contains'),
    '#default_value' => isset($param['name']) ? $param['name'] : '',
    '#weight' => 10,
  );
  
  $form['filters']['actions']['#type'] = 'actions';
  $form['filters']['actions']['#weight'] = 100;
  $form['filters']['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Filter'),
    '#weight' => 10,
    '#submit' => array('customer_admin_overview_submit'),
  );
  $form['filters']['actions']['reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset'),
    '#weight' => 20,
    '#submit' => array('customer_admin_overview_submit'),
  );
  
  $header = array(
    'name' => array(
      'data' => t('Name (Company or person)'),
      'field' => 'name',
      'type' => 'property',
      'specifier' => 'name',
    ),
  );
  $header += array(
    'owner' => array(
      'data' => t('Owner'),
      'field' => 'uid_owner',
      'type' => 'property',
      'specifier' => 'uid_owner',
    ),
  );
  if (field_info_instance('customer', 'email', 'customer') !== NULL) {
    $header += array(
      'email' => array(
        'data' => t('E-Mail'),
        'field' => 'email',
        'type' => 'field',
        'specifier' => array('field' => 'email', 'column' => 'email'),
      ),
    );
  }
  if (field_info_instance('customer', 'phone', 'customer') !== NULL) {
    $header += array(
      'phone' => array(
        'data' => t('Telephone number'),
        'field' => 'phone',
        'type' => 'field',
        'specifier' => array('field' => 'phone', 'column' => 'value'),
      ),
    );
  }
  if ($user_admin_access) {
    $header += array(
      'operations' => array(
        'data' => t('Operations'),
      ),
    );
  }
  
  $rows = array();
  
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'customer')
    ->tableSort($header)
    ->pager(10)
    ->addMetaData('account', user_load(1));
  if (isset($param['name'])) {
    $query->propertyCondition('name', $param['name'], 'CONTAINS');
  }
  $result = $query->execute();
  if (!empty($result)) {
    foreach ($result['customer'] as $item) {
      $wrappedCustomer = entity_metadata_wrapper('customer', $item->customer_id);
      $row = array(
        'name' => l($wrappedCustomer->label(), 'admin/customer/' . $wrappedCustomer->getIdentifier()),
        'owner' => is_object($wrappedCustomer->uid_owner->value()) ? theme('username', array('account' => $wrappedCustomer->uid_owner->value())) : '',
      );
      if (isset($header['email'])) {
        $field_view = field_view_field('customer', $wrappedCustomer->value(), 'email', array('label' => 'hidden'));
        $row['email'] = drupal_render($field_view);
      }
      if (isset($header['phone'])) {
        $field_view = field_view_field('customer', $wrappedCustomer->value(), 'phone', array('label' => 'hidden'));
        $row['phone'] = drupal_render($field_view);
      }
      if (isset($header['operations'])) {
        $row['operations'] = $user_admin_access ? l(t('Edit'), 'admin/customer/' . $wrappedCustomer->getIdentifier() . '/edit') : '';
      }
      $rows[$wrappedCustomer->getIdentifier()] = $row; // Store the entity id into the row's key for post-processed identification.
    }
  }
  $form['customers'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('Currently there are no customer informations.') . ' ' . l(t('Add new customer'), 'admin/customers/new-customer'),
  );
  $form['pager'] = array('#markup' => theme('pager'));
  drupal_add_js('misc/form.js');
  drupal_add_js('misc/collapse.js');
  return $form;
}

function customer_admin_overview_submit($form, &$form_state) {
  $values = $form_state['values'];
  if ($values['op'] == t('Reset')) {
    $form_state['redirect'] = current_path();
  }
  else {
    unset($values['submit'], $values['reset'], $values['form_build_id'], $values['form_token'], $values['form_id'], $values['op']);
    $form_state['redirect'] = array(current_path(), array('query' => array($values)));
  }
}
