/**
 * MikoJS TypeScript Definitions
 * Type definitions for the MikoJS JavaScript runtime and standard library
 */

// Global types
declare global {
    /**
     * Global console object
     */
    const console: {
        log(...args: any[]): void;
        error(...args: any[]): void;
        warn(...args: any[]): void;
    };

    /**
     * Global JSON object
     */
    const JSON: {
        parse(text: string): any;
        stringify(value: any): string;
    };

    /**
     * Global timer functions
     */
    function setTimeout(callback: () => void, delay: number): number;
    function clearTimeout(id: number): void;
    function setInterval(callback: () => void, interval: number): number;
    function clearInterval(id: number): void;

    /**
     * Global process object
     */
    const process: {
        exit(code?: number): void;
        argv(): string[];
        env(): { [key: string]: string };
    };

    /**
     * Global object
     */
    const global: any;

    /**
     * Module system
     */
    function require(id: string): any;
    const module: {
        exports: any;
    };
    const exports: any;
}

// Filesystem module types
export interface FileStats {
    size: number;
    isFile: boolean;
    isDirectory: boolean;
    mtime: number;
    ctime: number;
}

export interface FileSystem {
    /**
     * Read file contents synchronously
     */
    readFile(path: string): string;
    
    /**
     * Write content to file synchronously
     */
    writeFile(path: string, content: string): boolean;
    
    /**
     * Check if file or directory exists
     */
    exists(path: string): boolean;
    
    /**
     * Get file statistics
     */
    stat(path: string): FileStats;
    
    /**
     * Create directory
     */
    mkdir(path: string): boolean;
    
    /**
     * Remove file or directory
     */
    remove(path: string): boolean;
    
    /**
     * List directory contents
     */
    readdir(path: string): string[];
}

// Utilities module types
export interface TypeUtils {
    isString(value: any): value is string;
    isNumber(value: any): value is number;
    isBoolean(value: any): value is boolean;
    isArray(value: any): value is any[];
    isObject(value: any): value is object;
    isNullOrUndefined(value: any): value is null | undefined;
}

export interface ArrayUtils {
    max(arr: number[]): number | undefined;
    min(arr: number[]): number | undefined;
    sum(arr: number[]): number;
    unique<T>(arr: T[]): T[];
}

export interface StringUtils {
    capitalize(str: string): string;
    reverse(str: string): string;
    contains(str: string, substr: string): boolean;
    repeat(str: string, count: number): string;
}

export interface MathUtils {
    clamp(value: number, min: number, max: number): number;
    randomInt(min: number, max: number): number;
    isEven(num: number): boolean;
    isOdd(num: number): boolean;
}

export interface Utils {
    type: TypeUtils;
    array: ArrayUtils;
    string: StringUtils;
    math: MathUtils;
}

// Timer utilities
export interface Timers {
    setTimeout(callback: () => void, delay: number): number;
    clearTimeout(id: number): void;
    setInterval(callback: () => void, interval: number): number;
    clearInterval(id: number): void;
}

// Console utilities
export interface Console {
    log(...args: any[]): void;
    error(...args: any[]): void;
    warn(...args: any[]): void;
}

// JSON utilities
export interface JSONUtils {
    parse(text: string): any;
    stringify(value: any): string;
}

// Process utilities
export interface Process {
    exit(code?: number): void;
    argv(): string[];
    env(): { [key: string]: string };
}

// Main library interface
export interface MikoJSLibrary {
    fs: FileSystem;
    utils: Utils;
    console: Console;
    JSON: JSONUtils;
    timers: Timers;
    process: Process;
}

// Module declarations
declare module 'mikojs' {
    const lib: MikoJSLibrary;
    export = lib;
}

declare module 'mikojs/fs' {
    const fs: FileSystem;
    export = fs;
}

declare module 'mikojs/utils' {
    const utils: Utils;
    export = utils;
}

// Native function declarations (for internal use)
declare function __native_fs_readFile(path: string): string;
declare function __native_fs_writeFile(path: string, content: string): boolean;
declare function __native_fs_exists(path: string): boolean;
declare function __native_fs_stat(path: string): FileStats;
declare function __native_fs_mkdir(path: string): boolean;
declare function __native_fs_remove(path: string): boolean;
declare function __native_fs_readdir(path: string): string[];

declare function __native_console_log(message: string): void;
declare function __native_console_error(message: string): void;
declare function __native_console_warn(message: string): void;

declare function __native_json_parse(text: string): any;
declare function __native_json_stringify(value: any): string;

declare function __native_setTimeout(callback: () => void, delay: number): number;
declare function __native_clearTimeout(id: number): void;
declare function __native_setInterval(callback: () => void, interval: number): number;
declare function __native_clearInterval(id: number): void;

declare function __native_process_exit(code: number): void;
declare function __native_process_argv(): string[];
declare function __native_process_env(): { [key: string]: string };

// Export everything
export {
    FileSystem,
    FileStats,
    Utils,
    TypeUtils,
    ArrayUtils,
    StringUtils,
    MathUtils,
    Timers,
    Console,
    JSONUtils,
    Process,
    MikoJSLibrary
};