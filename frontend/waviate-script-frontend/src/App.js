import React, { useState } from 'react';
import Editor from '@monaco-editor/react';
import './App.css';

function App() {
  const [sidebarOpen, setSidebarOpen] = useState(true);
  const [activeTab, setActiveTab] = useState('explorer');
  const [files, setFiles] = useState([
    { id: 1, name: 'shader.c', type: 'file', language: 'c', content: '// shader.c\n#include "WaviateInput.h"\n\nfloat sample_process(const WaviateSampleInput* in, WaviateSampleStateWriter* out) {\n    return in->inputDeviceSamples[0][in->positionInBlock];\n}\n' },
    { id: 2, name: 'config.json', type: 'file', language: 'json', content: '{\n  "version": "1.0.0",\n  "name": "My Shader"\n}\n' },
  ]);
  const [openedFile, setOpenedFile] = useState(null);
  const [fileContents, setFileContents] = useState({});

  const tabs = [
    { id: 'explorer', icon: '📁', label: 'Explorer' },
    { id: 'search', icon: '🔍', label: 'Search' },
    { id: 'git', icon: '🌿', label: 'Source Control' },
    { id: 'debug', icon: '🐛', label: 'Debug' },
    { id: 'extensions', icon: '📦', label: 'Extensions' },
  ];

  const handleFileClick = (file) => {
    setOpenedFile(file);
    // Initialize file content if not already loaded
    if (!fileContents[file.id]) {
      setFileContents({ ...fileContents, [file.id]: file.content || '' });
    }
  };

  const handleEditorChange = (value) => {
    if (openedFile) {
      setFileContents({ ...fileContents, [openedFile.id]: value });
    }
  };

  const handleCloseFile = () => {
    setOpenedFile(null);
  };

  return (
    <div className="vs-code-container">
      {/* Activity Bar */}
      <div className="activity-bar">
        {tabs.map((tab) => (
          <button
            key={tab.id}
            className={`activity-bar-item ${activeTab === tab.id ? 'active' : ''}`}
            onClick={() => setActiveTab(tab.id)}
            title={tab.label}
          >
            <span className="activity-bar-icon">{tab.icon}</span>
          </button>
        ))}
      </div>

      {/* Main Container */}
      <div className="main-container">
        {/* Sidebar */}
        {sidebarOpen && (
          <div className="sidebar">
            <div className="sidebar-header">
              <h3>{tabs.find(t => t.id === activeTab)?.label}</h3>
            </div>
            <div className="sidebar-content">
              {activeTab === 'explorer' && (
                <div className="explorer">
                  <div className="explorer-section">
                    <div className="section-title">WAVIATE SCRIPT</div>
                    <ul className="file-tree">
                      {files.map((file) => (
                        <li
                          key={file.id}
                          className={`file-item ${openedFile?.id === file.id ? 'active' : ''}`}
                          onClick={() => handleFileClick(file)}
                        >
                          <span className="file-icon">{file.type === 'file' ? '📄' : '📁'}</span>
                          <span className="file-name">{file.name}</span>
                        </li>
                      ))}
                    </ul>
                  </div>
                </div>
              )}
              {activeTab === 'search' && (
                <div className="search-panel">
                  <input
                    type="text"
                    placeholder="Search"
                    className="search-input"
                  />
                </div>
              )}
              {activeTab === 'git' && (
                <div className="git-panel">
                  <p className="panel-message">No repository</p>
                </div>
              )}
              {activeTab === 'debug' && (
                <div className="debug-panel">
                  <p className="panel-message">Debug configurations</p>
                </div>
              )}
              {activeTab === 'extensions' && (
                <div className="extensions-panel">
                  <p className="panel-message">No extensions installed</p>
                </div>
              )}
            </div>
          </div>
        )}

        {/* Editor Area */}
        <div className="editor-area">
          <div className="tabs-bar">
            {openedFile && (
              <div className="tab-item active">
                <span className="tab-icon">📄</span>
                <span className="tab-name">{openedFile.name}</span>
                <button
                  className="tab-close"
                  onClick={handleCloseFile}
                >
                  ✕
                </button>
              </div>
            )}
          </div>

          <div className="editor-content">
            {openedFile ? (
              <Editor
                height="100%"
                language={openedFile.language || 'plaintext'}
                value={fileContents[openedFile.id] || ''}
                onChange={handleEditorChange}
                theme="vs-dark"
                options={{
                  minimap: { enabled: true },
                  fontSize: 13,
                  fontFamily: "'Consolas', 'Monaco', 'Courier New', monospace",
                  lineNumbers: 'on',
                  scrollBeyondLastLine: false,
                  automaticLayout: true,
                  smoothScrolling: true,
                  cursorBlinking: 'blink',
                  renderLineHighlight: 'all',
                  renderWhitespace: 'selection',
                }}
              />
            ) : (
              <div className="welcome-page">
                <div className="welcome-content">
                  <h1>Welcome to Waviate Script</h1>
                  <p>Open a file to start editing</p>
                </div>
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Status Bar */}
      <div className="status-bar">
        <div className="status-left">
          {openedFile && (
            <>
              <span>Ln 1, Col 1</span>
              <span>{openedFile.language || 'plaintext'}</span>
              <span>UTF-8</span>
            </>
          )}
        </div>
        <div className="status-right">
          <span>Waviate Script v1.0</span>
        </div>
      </div>
    </div>
  );
}

export default App;
