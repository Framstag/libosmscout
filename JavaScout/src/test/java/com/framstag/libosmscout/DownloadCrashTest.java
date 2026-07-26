package com.framstag.libosmscout;

import com.framstag.libosmscout.client.AvailableMapEntry;
import com.framstag.libosmscout.client.MapDownloadListener;
import com.framstag.libosmscout.client.MapDownloadManager;
import com.framstag.libosmscout.client.MapProvider;
import com.framstag.libosmscout.client.OSMScoutClient;
import com.framstag.libosmscout.client.OSMScoutClientBuilder;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class DownloadCrashTest {
    public static void main(String[] args) throws Exception {
        Path tmp = Files.createTempDirectory("javascout-download-test");
        System.out.println("[test] tmp dir: " + tmp);

        OSMScoutClientBuilder builder = new OSMScoutClientBuilder()
            .withMapLookupDirectories(tmp.toString())
            .withStyleSheetDirectory("/home/tim/projects/libosmscout/stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .withMapsDirectory(tmp.resolve("maps").toString());

        OSMScoutClient client = builder.build();
        if (client == null) {
            System.err.println("[test] builder returned null");
            System.exit(1);
        }

        MapDownloadManager mgr = client.getMapDownloadManager();
        MapProvider provider = new MapProvider(
            "karry.cz",
            "https://osmscout.karry.cz",
            "https://osmscout.karry.cz/latest.php?fromVersion=%1&toVersion=%2&locale=%3"
        );

        System.out.println("[test] fetching available maps...");
        List<AvailableMapEntry> entries = mgr.fetchAvailableMaps(provider);
        System.out.println("[test] fetched " + entries.size() + " top-level entries");

        AvailableMapEntry target = null;
        for (AvailableMapEntry e : entries) {
            if (!e.isDirectory()) {
                target = e;
                break;
            }
        }

        if (target == null) {
            System.err.println("[test] no downloadable map entry found");
            client.close();
            System.exit(1);
        }

        System.out.println("[test] selected map: " + target.getName());
        Path downloadDir = tmp.resolve("maps").resolve(target.getName());
        Files.createDirectories(downloadDir);

        CountDownLatch latch = new CountDownLatch(1);
        MapDownloadListener listener = new MapDownloadListener() {
            @Override
            public void onProgress(String name, long bytes, long total) {
                System.out.println("[test] progress " + name + ": " + bytes + "/" + total);
            }

            @Override
            public void onComplete(String name, String path) {
                System.out.println("[test] complete " + name + " -> " + path);
                latch.countDown();
            }

            @Override
            public void onError(String name, String message) {
                System.err.println("[test] error " + name + ": " + message);
                latch.countDown();
            }
        };

        System.out.println("[test] starting download...");
        String handle = mgr.downloadMap(target, downloadDir, listener);
        System.out.println("[test] download handle: " + handle);

        boolean completed = latch.await(5, TimeUnit.MINUTES);
        System.out.println("[test] completed within timeout: " + completed);

        client.close();
        System.out.println("[test] done");
    }
}
