<?php

/**
 * @file
 * Provides the customer object.
 */
 
/**
 * Customer class.
 */
class Customer extends Entity {
  protected function defaultLabel() {
    return $this->name;
  }
  protected function defaultUri() {
    return array('path' => 'customer/' . $this->identifier());
  }
}

/** 
 * CustomerController class.
 */
class CustomerController extends EntityAPIController {
  public function create(array $values = array()) {
    global $user;
    if (!isset($values['name'])) {
      $values['name'] = '';
    }
    if (!isset($values['created'])) {
      $values['created'] = REQUEST_TIME;
    }
    if (!isset($values['changed'])) {
      $values['changed'] = 0;
    }
    if (!isset($values['uid_created'])) {
      $values['uid_created'] = $user->uid;
    }
    if (!isset($values['uid_changed'])) {
      $values['uid_changed'] = 0;
    }
    if (!isset($values['uid_owner'])) {
      $values['uid_owner'] = 0;
    }
    return parent::create($values);
  }
}
