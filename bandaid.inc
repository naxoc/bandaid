<?php

/**
 * @file
 * Helper classes for Bandaid.
 */

namespace Bandaid;

/**
 * Bandaid error class.
 */
class BandaidError extends \Exception {
  protected $error;

  /**
   * Constuctor.
   */
  public function __construct($error, $message = '') {
    $this->error = $error;
    parent::__construct($message);
  }

  /**
   * Set error.
   *
   * Calls drush_set_error().
   */
  public function drushSetError() {
    if (($previous = $this->getPrevious()) && ($previous instanceof BandaidError)) {
      $previous->drushSetError();
    }
    // The error has already been set.
    if ($this->error == 'drush_set_error') {
      return FALSE;
    }
    $message = !empty($this->message) ? $this->message : NULL;
    return drush_set_error($this->error, $message);
  }
}

/**
 * Encapsulates Bandaids git operations.
 */
class Git {
  /**
   * Run a command.
   */
  protected static function exec($command, $args = array(), $error_code = 'GIT_EXEC_ERROR', $error_msg = "Error executing command \"!command\"") {
    array_unshift($args, $command);
    $result = call_user_func_array('drush_shell_exec', $args);
    if (!$result) {
      throw new BandaidError($error_code, strtr($error_msg, array('!command' => $command)));
    }
    return drush_shell_exec_output();
  }

  /**
   * Check if a patch will apply.
   *
   * @return bool
   *   Whether git thinks it will apply.
   */
  public static function applyCheck($filename, $level = 1) {
    try {
      static::exec('GIT_DIR=. git apply --check -p%s %s --verbose', array($level, $filename));
      return TRUE;
    }
    catch (BandaidError $e) {
      return FALSE;
    }
  }

  /**
   * Apply a patch.
   *
   * @return bool
   *   TRUE if the patch was applied successfully.
   */
  public static function apply($filename, $level = 1) {
    try {
      static::exec('GIT_DIR=. git apply -p%s %s --verbose', array($level, $filename));
      return TRUE;
    }
    catch (BandaidError $e) {
      return FALSE;
    }
  }

  /**
   * Get the oneline log between 2 commits.
   */
  public static function onelineLog($start, $end) {
    return static::exec('git log --format=oneline %s..%s', array($start, $end), 'CANNOT_GET_HISTORY');
  }

  /**
   * Checkout branch/tag/sha.
   */
  public static function checkout($ref) {
    return static::exec('git checkout %s', array($ref), 'CANNOT_CHECKOUT');
  }

  /**
   * Create a branch.
   */
  public static function branch($name) {
    return static::exec('git branch %s', array($name), 'CANNOT_BRANCH');
  }

  /**
   * Add files to index.
   */
  public static function add($pattern, $all = FALSE) {
    $all = $all ? '--all' : '';
    return static::exec('git add %s %s', array($all, $pattern), 'CANNOT_ADD');
  }

  /**
   * Commit changes.
   */
  public static function commit($message) {
    return static::exec('git commit -m %s', array($message), 'CANNOT_COMMIT');
  }

  /**
   * Return a diff.
   */
  public static function diff($ref1, $ref2 = '') {
    $patch = static::exec('git diff %s %s', array($ref1, $ref2), 'CANNOT_DIFF');
    // Due to going through drush_shell_exec_output(), we lost the trailing
    // newline.
    $patch = implode("\n", $patch);
    return empty($patch) ? '' : $patch . "\n";
  }

  /**
   * Reset workspace.
   */
  public static function reset($hard = FALSE) {
    $hard = $hard ? '--hard' : '';
    return static::exec('git reset %s', array($hard), 'CANNOT_RESET');
  }

  /**
   * Clone repository.
   */
  public static function cloneRepository($repository, $path, $options = array()) {
    $opt = array();
    foreach ($options as $name => $value) {
      if ($value === TRUE || $value === NULL) {
        $opt[] = '--' . $name;
      }
      else {
        $opt[] = '--' . $name . ' ' . drush_escapeshellarg($value);
      }
    }
    $command = 'git clone ' . implode(' ', $opt) . ' %s %s';
    return static::exec($command, array($repository, $path), 'CANNOT_CLONE', dt('Unable to clone from @url.', array('@url' => $url)));
  }

  /**
   * Run remote update.
   */
  public static function remoteUpdate() {
    static::exec('git remote update', array(), 'ERROR_UPDATING');
  }
}
