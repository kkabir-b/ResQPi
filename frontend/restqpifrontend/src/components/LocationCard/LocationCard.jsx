"use client";

import styles from "./LocationCard.module.css";
import StatusBadge from "../StatusBadge/StatusBadge";

export default function LocationCard({ location, onReset }) {
    return (
        <div
            className={`${styles.card} ${
                location.emergency ? styles.danger : styles.safe
            }`}
        >
            <div className={styles.header}>

                <div className={styles.info}>
                    <h2 className={styles.title}>
                        {location.name}
                    </h2>

                    <StatusBadge emergency={location.emergency} />
                </div>

                <div className={styles.icon}>
                    {location.emergency ? "🚨" : "✅"}
                </div>

            </div>

            {location.emergency && (
                <button
                    className={styles.resetButton}
                    onClick={() => onReset(location.name)}
                >
                    Reset Emergency
                </button>
            )}
        </div>
    );
}