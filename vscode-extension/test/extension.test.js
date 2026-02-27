const assert = require('assert');

// You can import and use all API from the 'vscode' module
// as well as import your extension to test it
const vscode = require('vscode');

suite('Extension Test Suite', () => {
    vscode.window.showInformationMessage('Start all tests.');

    test('Extension loads successfully', () => {
        const ext = vscode.extensions.getExtension('waviate.waviate-script');
        assert.ok(ext, 'Extension should be loaded');
    });

    test('Waviate C language is recognized', async () => {
        const languages = vscode.extensions.all
            .find(ext => ext.id === 'waviate.waviate-script')
            ?.packageJSON?.contributes?.languages || [];
        
        const wcLanguage = languages.find(l => l.id === 'waviate-c');
        assert.ok(wcLanguage, '.wc language should be registered');
        assert.ok(wcLanguage.extensions.includes('.wc'), '.wc should be associated');
    });

    test('Waviate C++ language is recognized', async () => {
        const languages = vscode.extensions.all
            .find(ext => ext.id === 'waviate.waviate-script')
            ?.packageJSON?.contributes?.languages || [];
        
        const wcppLanguage = languages.find(l => l.id === 'waviate-cpp');
        assert.ok(wcppLanguage, '.wcpp language should be registered');
        assert.ok(wcppLanguage.extensions.includes('.wcpp'), '.wcpp should be associated');
    });

    test('Waviate Rust language is recognized', async () => {
        const languages = vscode.extensions.all
            .find(ext => ext.id === 'waviate.waviate-script')
            ?.packageJSON?.contributes?.languages || [];
        
        const wrsLanguage = languages.find(l => l.id === 'waviate-rust');
        assert.ok(wrsLanguage, '.wrs language should be registered');
        assert.ok(wrsLanguage.extensions.includes('.wrs'), '.wrs should be associated');
    });

    test('Snippets are registered', async () => {
        const snippets = vscode.extensions.all
            .find(ext => ext.id === 'waviate.waviate-script')
            ?.packageJSON?.contributes?.snippets || [];
        
        assert.ok(snippets.length > 0, 'Snippets should be registered');
        assert.ok(snippets.some(s => s.language === 'waviate-c'), 'C snippets should exist');
        assert.ok(snippets.some(s => s.language === 'waviate-cpp'), 'C++ snippets should exist');
        assert.ok(snippets.some(s => s.language === 'waviate-rust'), 'Rust snippets should exist');
    });
});
