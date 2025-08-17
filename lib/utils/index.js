/**
 * MikoJS Utilities Library
 * Provides common utility functions
 */

/**
 * Type checking utilities
 */
const type = {
    /**
     * Check if value is a string
     * @param {*} value - Value to check
     * @returns {boolean} True if string
     */
    isString: function(value) {
        return typeof value === 'string';
    },

    /**
     * Check if value is a number
     * @param {*} value - Value to check
     * @returns {boolean} True if number
     */
    isNumber: function(value) {
        return typeof value === 'number' && !isNaN(value);
    },

    /**
     * Check if value is a boolean
     * @param {*} value - Value to check
     * @returns {boolean} True if boolean
     */
    isBoolean: function(value) {
        return typeof value === 'boolean';
    },

    /**
     * Check if value is an array
     * @param {*} value - Value to check
     * @returns {boolean} True if array
     */
    isArray: function(value) {
        return Array.isArray(value);
    },

    /**
     * Check if value is an object
     * @param {*} value - Value to check
     * @returns {boolean} True if object
     */
    isObject: function(value) {
        return typeof value === 'object' && value !== null && !Array.isArray(value);
    },

    /**
     * Check if value is null or undefined
     * @param {*} value - Value to check
     * @returns {boolean} True if null or undefined
     */
    isNullOrUndefined: function(value) {
        return value === null || value === undefined;
    }
};

/**
 * Array utilities
 */
const array = {
    /**
     * Find the maximum value in an array
     * @param {array} arr - Array of numbers
     * @returns {number} Maximum value
     */
    max: function(arr) {
        if (!Array.isArray(arr) || arr.length === 0) return undefined;
        let max = arr[0];
        for (let i = 1; i < arr.length; i++) {
            if (arr[i] > max) max = arr[i];
        }
        return max;
    },

    /**
     * Find the minimum value in an array
     * @param {array} arr - Array of numbers
     * @returns {number} Minimum value
     */
    min: function(arr) {
        if (!Array.isArray(arr) || arr.length === 0) return undefined;
        let min = arr[0];
        for (let i = 1; i < arr.length; i++) {
            if (arr[i] < min) min = arr[i];
        }
        return min;
    },

    /**
     * Sum all values in an array
     * @param {array} arr - Array of numbers
     * @returns {number} Sum of all values
     */
    sum: function(arr) {
        if (!Array.isArray(arr)) return 0;
        let sum = 0;
        for (let i = 0; i < arr.length; i++) {
            sum += arr[i];
        }
        return sum;
    },

    /**
     * Remove duplicates from array
     * @param {array} arr - Input array
     * @returns {array} Array without duplicates
     */
    unique: function(arr) {
        if (!Array.isArray(arr)) return [];
        let result = [];
        for (let i = 0; i < arr.length; i++) {
            let found = false;
            for (let j = 0; j < result.length; j++) {
                if (result[j] === arr[i]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                result.push(arr[i]);
            }
        }
        return result;
    }
};

/**
 * String utilities
 */
const string = {
    /**
     * Capitalize first letter of string
     * @param {string} str - Input string
     * @returns {string} Capitalized string
     */
    capitalize: function(str) {
        if (typeof str !== 'string' || str.length === 0) return str;
        return str.charAt(0).toUpperCase() + str.slice(1);
    },

    /**
     * Reverse a string
     * @param {string} str - Input string
     * @returns {string} Reversed string
     */
    reverse: function(str) {
        if (typeof str !== 'string') return str;
        let result = '';
        for (let i = str.length - 1; i >= 0; i--) {
            result += str.charAt(i);
        }
        return result;
    },

    /**
     * Check if string contains substring
     * @param {string} str - String to search in
     * @param {string} substr - Substring to find
     * @returns {boolean} True if contains substring
     */
    contains: function(str, substr) {
        if (typeof str !== 'string' || typeof substr !== 'string') return false;
        return str.indexOf(substr) !== -1;
    },

    /**
     * Repeat string n times
     * @param {string} str - String to repeat
     * @param {number} count - Number of repetitions
     * @returns {string} Repeated string
     */
    repeat: function(str, count) {
        if (typeof str !== 'string' || typeof count !== 'number' || count < 0) return '';
        let result = '';
        for (let i = 0; i < count; i++) {
            result += str;
        }
        return result;
    }
};

/**
 * Math utilities
 */
const math = {
    /**
     * Clamp value between min and max
     * @param {number} value - Value to clamp
     * @param {number} min - Minimum value
     * @param {number} max - Maximum value
     * @returns {number} Clamped value
     */
    clamp: function(value, min, max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    },

    /**
     * Generate random integer between min and max (inclusive)
     * @param {number} min - Minimum value
     * @param {number} max - Maximum value
     * @returns {number} Random integer
     */
    randomInt: function(min, max) {
        return Math.floor(Math.random() * (max - min + 1)) + min;
    },

    /**
     * Check if number is even
     * @param {number} num - Number to check
     * @returns {boolean} True if even
     */
    isEven: function(num) {
        return typeof num === 'number' && num % 2 === 0;
    },

    /**
     * Check if number is odd
     * @param {number} num - Number to check
     * @returns {boolean} True if odd
     */
    isOdd: function(num) {
        return typeof num === 'number' && num % 2 !== 0;
    }
};

// Export all utilities
module.exports = {
    type,
    array,
    string,
    math
};