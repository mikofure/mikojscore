/**
 * MikoJS Standard Library
 * Main entry point for all library modules
 */

// Import all library modules
const fs = require('./fs');
const utils = require('./utils');

/**
 * Console utilities for debugging and output
 */
const console = {
    /**
     * Print message to stdout
     * @param {...*} args - Arguments to print
     */
    log: function(...args) {
        // Native implementation will be provided by the runtime
        __native_console_log(args.join(' '));
    },

    /**
     * Print error message to stderr
     * @param {...*} args - Arguments to print
     */
    error: function(...args) {
        // Native implementation will be provided by the runtime
        __native_console_error(args.join(' '));
    },

    /**
     * Print warning message
     * @param {...*} args - Arguments to print
     */
    warn: function(...args) {
        // Native implementation will be provided by the runtime
        __native_console_warn(args.join(' '));
    }
};

/**
 * JSON utilities
 */
const JSON = {
    /**
     * Parse JSON string
     * @param {string} str - JSON string to parse
     * @returns {*} Parsed object
     */
    parse: function(str) {
        // Native implementation will be provided by the runtime
        return __native_json_parse(str);
    },

    /**
     * Stringify object to JSON
     * @param {*} obj - Object to stringify
     * @returns {string} JSON string
     */
    stringify: function(obj) {
        // Native implementation will be provided by the runtime
        return __native_json_stringify(obj);
    }
};

/**
 * Global setTimeout and setInterval functions
 */
const timers = {
    /**
     * Execute function after delay
     * @param {function} callback - Function to execute
     * @param {number} delay - Delay in milliseconds
     * @returns {number} Timer ID
     */
    setTimeout: function(callback, delay) {
        // Native implementation will be provided by the runtime
        return __native_setTimeout(callback, delay);
    },

    /**
     * Clear timeout
     * @param {number} id - Timer ID to clear
     */
    clearTimeout: function(id) {
        // Native implementation will be provided by the runtime
        __native_clearTimeout(id);
    },

    /**
     * Execute function repeatedly at intervals
     * @param {function} callback - Function to execute
     * @param {number} interval - Interval in milliseconds
     * @returns {number} Timer ID
     */
    setInterval: function(callback, interval) {
        // Native implementation will be provided by the runtime
        return __native_setInterval(callback, interval);
    },

    /**
     * Clear interval
     * @param {number} id - Timer ID to clear
     */
    clearInterval: function(id) {
        // Native implementation will be provided by the runtime
        __native_clearInterval(id);
    }
};

/**
 * Process utilities
 */
const process = {
    /**
     * Exit the process
     * @param {number} code - Exit code (default: 0)
     */
    exit: function(code = 0) {
        // Native implementation will be provided by the runtime
        __native_process_exit(code);
    },

    /**
     * Get command line arguments
     * @returns {array} Array of command line arguments
     */
    argv: function() {
        // Native implementation will be provided by the runtime
        return __native_process_argv();
    },

    /**
     * Get environment variables
     * @returns {object} Environment variables object
     */
    env: function() {
        // Native implementation will be provided by the runtime
        return __native_process_env();
    }
};

// Export all modules and utilities
module.exports = {
    fs,
    utils,
    console,
    JSON,
    timers,
    process
};

// Also make some utilities globally available
global.console = console;
global.JSON = JSON;
global.setTimeout = timers.setTimeout;
global.clearTimeout = timers.clearTimeout;
global.setInterval = timers.setInterval;
global.clearInterval = timers.clearInterval;
global.process = process;