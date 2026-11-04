import './editor/styles.css';
import { PlanetForge } from './app/planetForge';

const root = document.getElementById('root');
if (!root) throw new Error('#root missing');
const app = new PlanetForge(root);
void app.start();

// Console handle for smoke checks and scripted authoring from the dev console.
(window as unknown as { forge: PlanetForge }).forge = app;
