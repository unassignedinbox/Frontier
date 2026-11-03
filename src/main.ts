import './styles.css';
import { FrontierApp } from './app/FrontierApp';

const root = document.getElementById('app');
if (!root) throw new Error('Frontier root element is missing');
const mode = root.dataset.mode === 'editor' ? 'editor' : 'viewer';
const app = new FrontierApp(root, mode);
app.start();

// Handy for console inspection and automated smoke checks.
(window as any).frontier = app;
