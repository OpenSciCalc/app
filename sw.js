const CACHE_NAME = 'wasm-offline-cache-v1';

// List every file your app needs to run offline
const ASSETS_TO_CACHE = [
  './',
  './index.html',
  './OpenBreadboard.js',
  './OpenBreadboard.wasm' // Replace with your actual Wasm filename
];

// 1. Install Event: Triggers on first visit (requires internet)
self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(CACHE_NAME).then((cache) => {
      console.log('Caching all assets for offline use...');
      return cache.addAll(ASSETS_TO_CACHE);
    }).then(() => self.skipWaiting()) // Forces activation immediately
  );
});

// 2. Activate Event: Cleans up old caches if the version name changes
self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((cacheNames) => {
      return Promise.all(
        cacheNames.map((cache) => {
          if (cache !== CACHE_NAME) {
            console.log('Clearing old cache:', cache);
            return caches.delete(cache);
          }
        })
      );
    }).then(() => self.clients.claim())
  );
});

// 3. Fetch Event: Intercepts network requests
self.addEventListener('fetch', (event) => {
  event.respondWith(
    // Try to serve the file from the local cache first
    caches.match(event.request).then((cachedResponse) => {
      if (cachedResponse) {
        return cachedResponse; // Found in cache! Works offline.
      }
      // If it's not in the cache, try to fetch it from the network
      return fetch(event.request);
    })
  );
});
