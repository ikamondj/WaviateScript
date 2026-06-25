import { spawn } from 'child_process';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const root = path.resolve(__dirname, '../../');

function runCommand(command, args, cwd) {
  return new Promise((resolve, reject) => {
    console.log(`\n[Task] Running: ${command} ${args.join(' ')}`);
    const proc = spawn(command, args, { cwd, stdio: 'inherit', shell: true });
    proc.on('close', code => {
      if (code !== 0) {
        reject(new Error(`Command failed with code ${code}`));
      } else {
        resolve();
      }
    });
  });
}

async function main() {
  const childProcs = [];
  try {
    // 1. Idempotently ensure DB is running
    const persistenceDir = path.join(root, 'persistence');
    await runCommand('powershell', ['-File', './scripts/Initialize-Database.ps1'], persistenceDir);

    // 2. Start Go app in server mode
    const backendDir = path.join(root, 'backend');
    console.log('\n[Task] Starting Go server...');
    const goProc = spawn('go', ['run', 'cmd/server/main.go'], { 
      cwd: backendDir, 
      stdio: 'inherit', 
      shell: true, 
      env: {
        ...process.env,
        MARKETPLACE_MODE: 'server',
        MARKETPLACE_LOCAL_ADMIN: '1',
        MARKETPLACE_DATA_DIR: path.join(root, 'persistence', 'data'),
      }
    });
    childProcs.push(goProc);
    goProc.on('error', (err) => console.error(`[Error] Go server failed to start: ${err}`));

    // Wait a brief moment to allow the server to output its startup messages
    await new Promise(r => setTimeout(r, 1500));

    // 3. Start Vite
    const frontendDir = path.join(root, 'frontend');
    console.log('\n[Task] Starting Vite dev server...');
    const viteProc = spawn('npx', ['vite', '--host', '127.0.0.1', '--mode', 'loc'], { 
      cwd: frontendDir, 
      stdio: 'inherit', 
      shell: true 
    });
    childProcs.push(viteProc);
    viteProc.on('error', (err) => console.error(`[Error] Vite failed to start: ${err}`));

    // Handle graceful shutdown
    const shutdown = () => {
      console.log('\nShutting down child processes...');
      for (const p of childProcs) {
        if (!p.killed) {
          // Send kill signal, on windows killing shell processes can be tricky
          // but we do our best here
          p.kill('SIGINT');
        }
      }
      process.exit();
    };

    process.on('SIGINT', shutdown);
    process.on('SIGTERM', shutdown);

  } catch (err) {
    console.error(err);
    for (const p of childProcs) {
      if (!p.killed) p.kill('SIGINT');
    }
    process.exit(1);
  }
}

main();
