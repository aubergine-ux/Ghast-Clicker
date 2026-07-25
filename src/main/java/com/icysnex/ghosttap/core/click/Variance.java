package com.icysnex.ghosttap.core.click;

import java.util.concurrent.ThreadLocalRandom;

public abstract class Variance {

    public static double gaussian(double mean, double standardDeviation) {
        ThreadLocalRandom random = ThreadLocalRandom.current();

        return mean + random.nextGaussian() * standardDeviation;
    }

    public static boolean chance(double probability) {
        ThreadLocalRandom random = ThreadLocalRandom.current();

        return random.nextDouble() < probability;
    }

    public static double range(double min, double max) {
        // nextDouble requires max > min; return the value when the window is empty.
        if (min >= max)
            return min;
        return ThreadLocalRandom.current().nextDouble(min, max);
    }


    public static double trend(double current, double volatility, double tension) {
        double move = range(-volatility, volatility);
        return (current + move) * (1.0 - tension);
    }
    
}
