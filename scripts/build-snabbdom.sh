#!/bin/bash

# Navigate to the snabbdom directory
cd ../libs/snabbdom

echo "Installing Snabbdom dependencies..."
npm install

# Install build dependencies for bundling
echo "Installing build dependencies..."
npm install --save-dev rollup @rollup/plugin-typescript @rollup/plugin-node-resolve typescript

echo "Building Snabbdom from TypeScript..."
npm run build

echo "Creating single JavaScript bundle for JSEngine using Rollup..."

# Create rollup config for building a single AMD bundle
cat > rollup.config.js << 'EOF'
import typescript from '@rollup/plugin-typescript';
import { nodeResolve } from '@rollup/plugin-node-resolve';

export default {
  input: 'src/index.ts',
  output: {
    file: 'dist/snabbdom.js',
    format: 'amd',
    name: 'snabbdom'
  },
  plugins: [
    nodeResolve(),
    typescript({
      compilerOptions: {
        target: 'ES5',
        declaration: false,
        sourceMap: false,
        lib: ['DOM', 'ES5'],
        skipLibCheck: true,
        outDir: 'dist'
      }
    })
  ]
};
EOF

# Create dist directory if it doesn't exist
mkdir -p dist

# Build with rollup
npx rollup -c rollup.config.js

# Move the output to the correct location if successful
if [ -f "dist/snabbdom.js" ]; then
    echo "Rollup build successful, moving to target location..."
    cp dist/snabbdom.js ../../resin/res/snabbdom.js
    echo "Snabbdom bundle copied to resin/res/snabbdom.js"
fi


# Clean up build files
rm -f rollup.config.js
rm -rf dist
