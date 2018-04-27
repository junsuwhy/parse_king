<?php
/**
 * Implements hook_drush_command().
 */
function parse_king_drush_command() {

  $items['drush-demo-command'] = array(
    'description' => 'Demonstrate how Drush commands work.',
    'aliases' => array('ddc'),
    'arguments' => array(
      'type' => 'The type of statement (error or success).',
    ),
    'options' => array(
      'repeat' => 'The number of statement repeats.',
    ),
  );
  $items['add-keyword'] = array(
    'aliases' => array('pkak'),
    'arguments' => array(
      'word' => 'The word you want to add',
    ),
  );
  $items['remove-keyword'] = array(
    'aliases' => array('pkrk'),
    'arguments' => array(
      'kid' => 'The keyword id you want to remove',
    ),
  );
  $items['show-keyword'] = array(
    'aliases' => array('pksk'),
  );
  $items['parse-google-result'] = array(
    'aliases' => array('pkpr'),
    'options' => array(
      'kid' => 'The keyword id',
    ),
  );

  return $items;
}

function drush_parse_king_add_keyword($word){
  $result = db_query("SELECT id FROM parse_king_keyword WHERE word = :word", array(':word' => $word))->fetchField();
  print_r($result);
  if(empty($result)){
    db_query("INSERT INTO parse_king_keyword (word, created_time) VALUES(:word, :time)", array(
      ':word' => $word,
      ':time' => REQUEST_TIME,
    ));
    print("$word has been added.\n");
  }else{
    print("There are already keyword as $word . id is $result.\n");
  }
}

function drush_parse_king_remove_keyword($kid){
  $id = db_query("SELECT id FROM parse_king_keyword WHERE id = :kid", array(':kid' => $kid))->fetchField();
  if(!empty($id)){
    db_query("DELETE FROM parse_king_keyword WHERE id = :id", array(':id' => $id));
    print("$word has been removed.\n");
  }else{
    print("There are no word is $word.\n");
  }
}

function drush_parse_king_show_keyword(){
  $results = db_query("SELECT id, word FROM parse_king_keyword")->fetchAll();
  foreach ($results as $o) {
    print($o->id.', '.$o->word."\n");
  }
}

function drush_parse_king_parse_google_result(){
  module_load_include('php', 'parse_king', 'simple_html_dom');
  // include_once(DRUPAL_ROOT . '/' . drupal_get_path('module', 'parse_king') . "/simple_html_dom");
  $kid = drush_get_option('kid');
  $did = drush_get_option('did');
  $result = db_select('search', 's')
    ->fields('s')
    ->condition('kid', $kid)
    ->condition('did', $did)
    ->execute()
    ->fetchAll();
  if(empty($result)){
    // Add a search data.
    $word = db_query("SELECT word FROM parse_king_keyword WHERE id = :kid", array(':kid' => $kid))
    ->fetchField();
    $html = file_get_html("https://www.google.com.tw/search?q=$word");
    foreach($html->find('.gr .g') as $result_obj){
      // each row add result data
      $url = $result_obj->find('link')->href;
      $title = $result_obj->find('h3.r a')->innertext;
      $link_obj = _parse_king_get_link_obj_from_url($url, TRUE);
    }
  }


  


}

function _parse_king_get_link_obj_from_url($url, $auto_create = FALSE){
  $link = db_query("SELECT * FROM parse_king_link WHERE url = :url", array(':url' => $url))->fetchObj();
  if(!empty($link)){
    return $link;
  }else if($auto_create){
    db_insert()
  }
}

/**
 * Callback for the drush-demo-command command
 */
function drush_parse_king_drush_demo_command($type) {

  // Check for existence of argument
  if (!$type) {
    $options = array(
      'success' => dt('Success'),
      'error' => dt('Error'),
    );
    $type = drush_choice($options, dt('What kind of message you\'d like to print?'));
  }

  $repeat = drush_get_option('repeat', 0);
  for ($i=0; $i < $repeat; $i++) { 
    print($type . "\n");
  }
}