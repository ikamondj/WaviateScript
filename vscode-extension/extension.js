const vscode = require('vscode');
const fs = require('fs');
const path = require('path');
const https = require('https');

function activate(context) {
    console.log('Waviate Script extension activated');

    // Register completion provider for Waviate API
    const completionProvider = vscode.languages.registerCompletionItemProvider(
        [
            { scheme: 'file', language: 'waviate-c' },
            { scheme: 'file', language: 'waviate-cpp' },
            { scheme: 'file', language: 'waviate-rust' }
        ],
        {
            provideCompletionItems(document, position, token) {
                return getWaviateCompletions(document, position);
            }
        },
        '.'
    );
    context.subscriptions.push(completionProvider);

    // Watch for file open events to generate c_cpp_properties.json
    vscode.workspace.onDidOpenTextDocument((doc) => {
        if (doc.languageId === 'waviate-c' || doc.languageId === 'waviate-cpp') {
            generateCppProperties(doc);
        }
    });

    // Also handle files that were already open when extension loads
    vscode.workspace.textDocuments.forEach((doc) => {
        if (doc.languageId === 'waviate-c' || doc.languageId === 'waviate-cpp') {
            generateCppProperties(doc);
        }
    });

    // Register hover provider for documentation
    const hoverProvider = vscode.languages.registerHoverProvider(
        [
            { scheme: 'file', language: 'waviate-c' },
            { scheme: 'file', language: 'waviate-cpp' }
        ],
        {
            provideHover(document, position, token) {
                return getWaviateHover(document, position);
            }
        }
    );
    context.subscriptions.push(hoverProvider);

    // Command to check/download Waviate headers
    const downloadCommand = vscode.commands.registerCommand('waviate.downloadHeaders', ensureWaviateHeaders);
    context.subscriptions.push(downloadCommand);
}

function deactivate() {
    console.log('Waviate Script extension deactivated');
}

/**
 * Get Waviate API completions
 */
function getWaviateCompletions(document, position) {
    const completions = [];
    
    // struct/type completions
    const structs = [
        { name: 'WaviateSampleInput', detail: 'Input structure for sample processing' },
        { name: 'WaviateSampleStateWriter', detail: 'State writer for sample processing' },
        { name: 'WaviateFrequencyInput', detail: 'Input structure for frequency processing' },
        { name: 'WaviateFrequencyStateWriter', detail: 'State writer for frequency processing' }
    ];

    // function completions
    const functions = [
        { name: 'sample_process', detail: 'Main sample processing function' },
        { name: 'frequency_process', detail: 'Main frequency processing function' }
    ];

    // field completions for WaviateSampleInput
    const sampleInputFields = [
        'samplesSinceAppStart',
        'positionInBlock',
        'blockSize',
        'midiSegmentSize',
        'inputChannelCount',
        'sideChainChannelCount',
        'sampleMemoryCount',
        'outputChannelCount',
        'outputChannel',
        'midiNoteOn',
        'midiCCValue',
        'sampleWhenMidiNoteOn',
        'sampleWhenMidiNoteOff',
        'sampleWhenCCValueChanged',
        'controllerCount',
        'controllerButtonMask',
        'sampleWhenControllerButtonChanged',
        'controllerButtonCount',
        'controllerAxisValue',
        'sampleWhenControllerAxisChanged',
        'controllerAxisCount',
        'sampleRate',
        'previousSamples',
        'inputDeviceSamples',
        'inputSideChainSamples',
        'currentSampleData'
    ];

    // Add struct completions
    structs.forEach(s => {
        const item = new vscode.CompletionItem(s.name, vscode.CompletionItemKind.Struct);
        item.detail = s.detail;
        completions.push(item);
    });

    // Add function completions
    functions.forEach(f => {
        const item = new vscode.CompletionItem(f.name, vscode.CompletionItemKind.Function);
        item.detail = f.detail;
        completions.push(item);
    });

    // Add field completions
    sampleInputFields.forEach(field => {
        const item = new vscode.CompletionItem(field, vscode.CompletionItemKind.Field);
        item.detail = 'WaviateSampleInput field';
        completions.push(item);
    });

    return completions;
}

/**
 * Get hover information for Waviate API
 */
function getWaviateHover(document, position) {
    const word = document.getWordRangeAtPosition(position);
    if (!word) return null;

    const text = document.getText(word);

    const documentation = {
        'WaviateSampleInput': 'Struct containing all input data for sample processing.\nIncludes MIDI, audio buffers, timing, and controller data.',
        'WaviateSampleStateWriter': 'Struct for writing state updates during sample processing.',
        'WaviateFrequencyInput': 'Struct containing input data for frequency-domain processing.',
        'WaviateFrequencyStateWriter': 'Struct for writing state updates during frequency processing.',
        'sample_process': 'Entry point for per-sample audio processing.\nSignature: float sample_process(const WaviateSampleInput* in, WaviateSampleStateWriter* out)',
        'frequency_process': 'Entry point for per-bin frequency processing.\nSignature: float frequency_process(const WaviateFrequencyInput* in, WaviateFrequencyStateWriter* out)',
        'samplesSinceAppStart': 'Monotonically increasing sample counter since engine start.',
        'positionInBlock': 'Index of current sample within block (0 to blockSize-1).',
        'blockSize': 'Number of samples in current processing block.',
        'inputDeviceSamples': 'Read-only audio input. Indexed as [channel][sample].',
        'currentSampleData': 'Output buffer. Indexed as [channel][sample].',
        'midiNoteOn': 'Array of 128 MIDI note states (0 or 1).',
        'midiCCValue': 'Array of 128 MIDI CC values (0-127).',
        'sampleRate': 'Audio sample rate in Hz (e.g., 44100.0).'
    };

    if (documentation[text]) {
        const md = new vscode.MarkdownString(documentation[text]);
        return new vscode.Hover(md);
    }

    return null;
}

/**
 * Ensure Waviate headers exist in the workspace
 */
async function ensureWaviateHeaders() {
    const workspaceFolder = vscode.workspace.workspaceFolders?.[0];
    if (!workspaceFolder) {
        vscode.window.showErrorMessage('No workspace folder open');
        return;
    }

    const sourceDir = path.join(workspaceFolder.uri.fsPath, 'Source');
    const waviateCPath = path.join(sourceDir, 'Waviate.h');
    const waviateHppPath = path.join(sourceDir, 'Waviate.hpp');

    // Check if headers already exist
    if (fs.existsSync(waviateCPath) && fs.existsSync(waviateHppPath)) {
        vscode.window.showInformationMessage('Waviate headers already present in Source folder');
        return;
    }

    // Try to download from GitHub
    try {
        vscode.window.showInformationMessage('Downloading Waviate headers from GitHub...');
        
        const headerUrl = 'https://raw.githubusercontent.com/ikamondj/WaviateScript/main/Source/Waviate.h';
        const hppUrl = 'https://raw.githubusercontent.com/ikamondj/WaviateScript/main/Source/Waviate.hpp';

        // Create Source directory if needed
        if (!fs.existsSync(sourceDir)) {
            fs.mkdirSync(sourceDir, { recursive: true });
        }

        let downloadedH = false;
        let downloadedHpp = false;

        // Download Waviate.h
        if (!fs.existsSync(waviateCPath)) {
            const hContent = await downloadFile(headerUrl);
            if (hContent) {
                fs.writeFileSync(waviateCPath, hContent);
                downloadedH = true;
            }
        } else {
            downloadedH = true;
        }

        // Download Waviate.hpp
        if (!fs.existsSync(waviateHppPath)) {
            const hppContent = await downloadFile(hppUrl);
            if (hppContent) {
                fs.writeFileSync(waviateHppPath, hppContent);
                downloadedHpp = true;
            }
        } else {
            downloadedHpp = true;
        }

        if (downloadedH && downloadedHpp) {
            vscode.window.showInformationMessage('Waviate headers downloaded successfully!');
        } else {
            vscode.window.showWarningMessage('Failed to download some Waviate headers');
        }
    } catch (err) {
        vscode.window.showErrorMessage('Error downloading Waviate headers: ' + err.message);
    }
}

/**
 * Download file from URL
 */
function downloadFile(url) {
    return new Promise((resolve, reject) => {
        https.get(url, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => resolve(data));
        }).on('error', reject);
    });
}

/**
 * Generate .vscode/c_cpp_properties.json for C++ intellisense
 */
function generateCppProperties(document) {
    const workspaceFolder = vscode.workspace.getWorkspaceFolder(document.uri);
    if (!workspaceFolder) return;

    const vscodeDirPath = path.join(workspaceFolder.uri.fsPath, '.vscode');
    const propertiesPath = path.join(vscodeDirPath, 'c_cpp_properties.json');
    
    // Point to Source folder in workspace
    const sourceDir = path.join(workspaceFolder.uri.fsPath, 'Source');
    
    const properties = {
        "configurations": [
            {
                "name": "Waviate Script",
                "includePath": [
                    "${workspaceFolder}/Source",
                    "${workspaceFolder}/**"
                ],
                "defines": [
                    "WAVIATE_SCRIPT"
                ],
                "forcedInclude": [
                    path.join(sourceDir, "Waviate.h").replace(/\\/g, '/')
                ],
                "compilerPath": "/usr/bin/clang",
                "cStandard": "c11",
                "cppStandard": "c++17",
                "intelliSenseMode": "clang-x64"
            }
        ],
        "version": 4
    };

    // Create .vscode directory if it doesn't exist
    if (!fs.existsSync(vscodeDirPath)) {
        fs.mkdirSync(vscodeDirPath, { recursive: true });
    }

    // Write the properties file
    try {
        fs.writeFileSync(propertiesPath, JSON.stringify(properties, null, 2));
        console.log('Generated c_cpp_properties.json at', propertiesPath);

        // Check if Waviate headers exist, prompt to download if not
        const waviateCPath = path.join(sourceDir, 'Waviate.h');
        if (!fs.existsSync(waviateCPath)) {
            vscode.window.showInformationMessage(
                'Waviate.h not found in Source folder. Download from GitHub?',
                'Download',
                'Cancel'
            ).then(selection => {
                if (selection === 'Download') {
                    ensureWaviateHeaders();
                }
            });
        }
    } catch (err) {
        console.error('Error writing c_cpp_properties.json:', err);
    }
}

module.exports = {
    activate,
    deactivate
};
