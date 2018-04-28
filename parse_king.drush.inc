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

  $items['add-domain'] = array(
    'aliases' => array('pkad'),
    'arguments' => array(
      'word' => 'The domain you want to add',
    ),
  );
  $items['remove-domain'] = array(
    'aliases' => array('pkrd'),
    'arguments' => array(
      'kid' => 'The keyword id you want to remove',
    ),
  );
  $items['show-domain'] = array(
    'aliases' => array('pksd'),
  );
  $items['parse-google-result'] = array(
    'aliases' => array('pkpr'),
    'options' => array(
      'kid' => 'The keyword id',
      'did' => 'The domain id',
      'force' => 'Force add',
    ),
  );

  return $items;
}

function drush_parse_king_add_keyword($word){
  $result = db_query("SELECT id FROM parse_king_keyword WHERE word = :word", array(':word' => $word))->fetchField();
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

function drush_parse_king_add_domain($domain, $name){
  $result = db_query("SELECT id FROM parse_king_domain WHERE domain = :domain", array(':domain' => $domain))->fetchField();
  if(empty($result)){
    db_query("INSERT INTO parse_king_domain (domain, name) VALUES(:domain, :name)", array(
      ':domain' => $domain,
      ':name' => $name,
    ));
    print("$domain has been added.\n");
  }else{
    print("There are already domain as $domain . id is $result.\n");
  }
}

function drush_parse_king_remove_domain($did){
  $id = db_query("SELECT id FROM parse_king_domain WHERE id = :did", array(':did' => $did))->fetchField();
  if(!empty($id)){
    db_query("DELETE FROM parse_king_domain WHERE id = :id", array(':id' => $id));
    print("$domain has been removed.\n");
  }else{
    print("There are no word is $domain.\n");
  }
}

function drush_parse_king_show_domain(){
  $results = db_query("SELECT id, domain FROM parse_king_domain")->fetchAll();
  foreach ($results as $o) {
    print($o->id.', '.$o->domain."\n");
  }
}

function drush_parse_king_parse_google_result(){
  module_load_include('php', 'parse_king', 'simple_html_dom');
  header('Content-Type: text/html; charset=utf-8');
  // include_once(DRUPAL_ROOT . '/' . drupal_get_path('module', 'parse_king') . "/simple_html_dom");
  $kid = drush_get_option('kid');
  $did = drush_get_option('did');
  $result = db_select('parse_king_search', 's')
    ->fields('s')
    ->condition('keyword_id', $kid)
    ->condition('domain_id', $did)
    ->execute()
    ->fetchAll();
  if(empty($result) || drush_get_option('force')){
    print("no data\n");
    // Add a search data.

    $sid = db_insert('parse_king_search')
      ->fields(array(
        'keyword_id' => $kid,
        'domain_id' => $did,
        'search_time' => REQUEST_TIME,
      ))
      ->execute();


    $word = db_query("SELECT word FROM parse_king_keyword WHERE id = :kid", array(':kid' => $kid))
    ->fetchField();
    $url = "https://www.google.com.tw/search?q=$word";
    if(!empty($did)){
      $domain = db_select("parse_king_domain", 'd')
        ->fields('d')
        ->condition("id", $did)
        ->execute()
        ->fetchObject();
      $url .= "+site%3A{$domain->domain}";
    }
    print("Search url : $url \n");
    $html = file_get_html($url);
    if(!empty($html)){
      $i = 1;
      foreach($html->find('.g') as $result_obj){
        // each row add result data
        $url = $result_obj->find('cite','0')->innertext;
        $url = utf8_encode($url);
        if(empty($url)){
          print($result_obj->innertext."\n");
          continue;
        }
        $title = $result_obj->find('h3.r a','0')->innertext;
        $title = utf8_encode($title);
        print("Get row: $title, $url, order = $i\n");

        $created_fields = array(
          "keyword_id" => $kid,
          "title" => $title,
          "source_domain_id" => $did,
        );
        $link = _parse_king_get_link_obj_from_url($url, TRUE, $created_fields);
        $result_values = array(
            "search_id" => $sid,
            "link_id" => $link->id,
            "`order`" => $i,
          );
        db_insert("parse_king_result")
          ->fields($result_values)
          ->execute();
        $i++;
      }
      print("success add search and results about $word \n");
    }else{
      print("Can't add search and results about $word \n");
    }
  }else{
    print("kid = $kid, Search result is exist.");
    // List search result;
  }
}

function _parse_king_get_link_obj_from_url($url, $auto_create = FALSE, $created_fields){
  $link = db_select("parse_king_link", "l")
    ->fields("l")
    ->condition("url", $url)
    ->execute()
    ->fetchObject();
  if(!empty($link)){
    return $link;
  }else if($auto_create){
    $all_created_fields = $created_fields + array('url' => $url);
    $lid = db_insert("parse_king_link")
      ->fields($all_created_fields)
      ->execute();
    $link = db_select("parse_king_link", "l")
      ->fields("l")
      ->execute()
      ->fetchObject();
    return $link;
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