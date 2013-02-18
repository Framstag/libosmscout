package osm.scout.benchmark;

public interface BenchmarkProgressListener {
	
	void onStart(int totalCount);
	void updateProgress(Integer... progress);
	void onEnd();

}
