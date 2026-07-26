package com.framstag.libosmscout.client;

/**
 * Callback interface for asynchronous route calculation.
 * <p>
 * Passed to {@link OSMScoutClient#calculateRouteAsync(double, double, double, double, RouteCallback)}.
 * Methods are invoked from the JNI native thread — implementations MUST
 * marshal to the UI thread (e.g. using Platform.runLater).
 */
public interface RouteCallback {

    /**
     * Called periodically during route calculation.
     *
     * @param percent progress percentage (0-100)
     */
    void onProgress(int percent);

    /**
     * Called when route calculation completes successfully.
     *
     * @param route the computed route with geometry and metadata
     */
    void onSuccess(RouteEntry route);

    /**
     * Called when route calculation fails.
     *
     * @param message human-readable error description
     */
    void onError(String message);

    /**
     * Called when route calculation is cancelled by the user.
     */
    void onCancel();
}
