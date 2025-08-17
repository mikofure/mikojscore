/**
 * MikoJS Filesystem Library
 * Provides basic file system operations
 */

/**
 * Read file contents synchronously
 * @param {string} path - File path to read
 * @returns {string} File contents
 */
function readFile(path) {
    // Native implementation will be provided by the runtime
    return __native_fs_readFile(path);
}

/**
 * Write content to file synchronously
 * @param {string} path - File path to write
 * @param {string} content - Content to write
 * @returns {boolean} Success status
 */
function writeFile(path, content) {
    // Native implementation will be provided by the runtime
    return __native_fs_writeFile(path, content);
}

/**
 * Check if file or directory exists
 * @param {string} path - Path to check
 * @returns {boolean} True if exists, false otherwise
 */
function exists(path) {
    // Native implementation will be provided by the runtime
    return __native_fs_exists(path);
}

/**
 * Get file statistics
 * @param {string} path - File path
 * @returns {object} File stats object
 */
function stat(path) {
    // Native implementation will be provided by the runtime
    return __native_fs_stat(path);
}

/**
 * Create directory
 * @param {string} path - Directory path to create
 * @returns {boolean} Success status
 */
function mkdir(path) {
    // Native implementation will be provided by the runtime
    return __native_fs_mkdir(path);
}

/**
 * Remove file or directory
 * @param {string} path - Path to remove
 * @returns {boolean} Success status
 */
function remove(path) {
    // Native implementation will be provided by the runtime
    return __native_fs_remove(path);
}

/**
 * List directory contents
 * @param {string} path - Directory path
 * @returns {array} Array of file/directory names
 */
function readdir(path) {
    // Native implementation will be provided by the runtime
    return __native_fs_readdir(path);
}

// Export all functions
module.exports = {
    readFile,
    writeFile,
    exists,
    stat,
    mkdir,
    remove,
    readdir
};